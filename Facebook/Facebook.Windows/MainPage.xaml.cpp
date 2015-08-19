//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

#include "WinRTFacebookSession.h"
#include "WinRTFacebookHelpers.h"

using namespace Facebook;

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

MainPage::MainPage()
{
	InitializeComponent();

	FBHelpers::s_HttpRequestManager = std::shared_ptr<IHttpRequestManager>(CreateHttpRequestManager());

	CWinRTFacebookSession::Initialise(
		Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher);
}


void Facebook::MainPage::FBLogout_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	CWinRTFacebookSession::Close();

	this->Output->Text += "Session Closed\n";
}


void Facebook::MainPage::FBLogin_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->ProgressBar->Visibility = ::Visibility::Visible;

	CWinRTFacebookSession::Open(L"710421129063177", L"email,user_likes")
		.then([=](bool session_open)
	{
		this->ProgressBar->Visibility = ::Visibility::Collapsed;

		if (session_open)
		{
			this->Output->Text += "Session Opened\n";
		}
		else
		{
			this->Output->Text += "Session Not Opened\n";
		}
	}, task_continuation_context::use_current());
}
