//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

#include "Facebook.h"
#include "WinRTFacebookHelpers.h"

using namespace Facebook;

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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();

	FBHelpers::s_HttpRequestManager = std::shared_ptr<IHttpRequestManager>(CreateHttpRequestManager());

	CWinRTFacebookClient::instance().initialise(
		L"710421129063177",
		Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher);
}


void Facebook::MainPage::FBLogout_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	CWinRTFacebookClient::instance().logout();
}


void Facebook::MainPage::FBLogin_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	CWinRTFacebookClient::instance().login("email,user_likes")
		.then([](bool logged_in)
	{
		logged_in;
	});
}
