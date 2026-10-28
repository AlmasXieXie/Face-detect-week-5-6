#pragma once
#include "FrameProcessor.h"
#include <msclr/marshal_cppstd.h>

namespace FaceDetectMask {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MyForm
	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form
	{
	public:
		MyForm(void)
		{
			InitializeComponent();
			proc = nullptr;
			running = false;
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::PictureBox^ pictureBox1;
	private: System::Windows::Forms::Label^ lblStatus;

	protected:

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->lblStatus = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
			this->SuspendLayout();
			// 
			// pictureBox1
			// 
			this->pictureBox1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->pictureBox1->Location = System::Drawing::Point(0, 0);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(1424, 626);
			this->pictureBox1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::Zoom;
			this->pictureBox1->TabIndex = 0;
			this->pictureBox1->TabStop = false;
			// 
			// lblStatus
			// 
			this->lblStatus->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->lblStatus->Location = System::Drawing::Point(0, 613);
			this->lblStatus->Name = L"lblStatus";
			this->lblStatus->Size = System::Drawing::Size(1424, 13);
			this->lblStatus->TabIndex = 1;
			this->lblStatus->Text = L"label1";
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1424, 626);
			this->Controls->Add(this->lblStatus);
			this->Controls->Add(this->pictureBox1);
			this->Name = L"MyForm";
			this->Text = L"MyForm";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &MyForm::MyForm_FormClosing);
			this->Load += gcnew System::EventHandler(this, &MyForm::MyForm_Load);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion

	private:
		FrameProcessor* proc;
		System::Threading::Thread^ worker;
		volatile bool running;
		delegate void ShowFrameDelegate(System::Drawing::Bitmap^ bmp, System::String^ text);

		void ShowFrame(System::Drawing::Bitmap^ bmp, System::String^ text) {
			if (bmp != nullptr) {
				System::Drawing::Image^ old = pictureBox1->Image;
				pictureBox1->Image = bmp;
				if (old != nullptr) delete old;
			}
			lblStatus->Text = text;
		}

		void Status(System::String^ text) {
			if (!this->IsDisposed)
				this->BeginInvoke(gcnew ShowFrameDelegate(this, &MyForm::ShowFrame), nullptr, text);
		}

		void CaptureLoop() {
			using namespace System;
			using namespace System::Drawing;
			using namespace System::Drawing::Imaging;
			std::vector<unsigned char> buf;
			while (running) {
				try {
					if (!proc->isOpen() && !proc->openCamera(0)) {
						Status("Камера не открылась");
						Threading::Thread::Sleep(2000);
						continue;
					}
					int w, h, m, n;
					if (!proc->next(w, h, buf, m, n)) {
						Status("Кадр с камеры не получен");
						proc->close();
						Threading::Thread::Sleep(1000);
						continue;
					}

					Bitmap^ bmp = gcnew Bitmap(w, h, PixelFormat::Format24bppRgb);
					BitmapData^ bd = bmp->LockBits(System::Drawing::Rectangle(0, 0, w, h),
						ImageLockMode::WriteOnly, PixelFormat::Format24bppRgb);
					for (int y = 0; y < h; y++)
						memcpy((unsigned char*)bd->Scan0.ToPointer() + y * bd->Stride, &buf[y * w * 3], w * 3);
					bmp->UnlockBits(bd);

					String^ text = String::Format("С маской: {0}   Без маски: {1}", m, n);
					if (!running || this->IsDisposed) { delete bmp; break; }
					this->BeginInvoke(gcnew ShowFrameDelegate(this, &MyForm::ShowFrame), bmp, text);
					Threading::Thread::Sleep(30);
				}
				catch (const std::exception& ex) {
					if (proc) proc->close();
					Status("Ошибка: " + gcnew String(ex.what()));
					Threading::Thread::Sleep(3000);
				}
			}
		}

	private: System::Void MyForm_Load(System::Object^ sender, System::EventArgs^ e) {
		msclr::interop::marshal_context ctx;
		std::string d = ctx.marshal_as<std::string>(System::AppDomain::CurrentDomain->BaseDirectory);
		try {
			proc = new FrameProcessor(d + "models\\deploy.prototxt",
				d + "models\\res10_300x300_ssd_iter_140000.caffemodel",
				d + "models\\mask_detector.onnx");
		}
		catch (const std::exception& ex) {
			MessageBox::Show(gcnew System::String(ex.what()), "Ошибка загрузки моделей");
			return;
		}
		running = true;
		worker = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(this, &MyForm::CaptureLoop));
		worker->IsBackground = true;
		worker->Start();
	}

	private: System::Void MyForm_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e) {
		running = false;
		if (worker != nullptr) worker->Join(3000);
		if (proc) {
			delete proc;
			proc = nullptr;
		}
	}
	};
}