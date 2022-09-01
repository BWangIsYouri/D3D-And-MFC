
// D3DMFCDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "D3DMFC.h"
#include "D3DMFCDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
static bool      mAppPaused = false;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CD3DMFCDlg 对话框



CD3DMFCDlg::CD3DMFCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_D3DMFC_DIALOG, pParent)
{
	
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CD3DMFCDlg::~CD3DMFCDlg()
{
	
}

void CD3DMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CD3DMFCDlg::PreTranslateMessage(MSG* pMsg)
{
	app->render();
	return 0;
}

BEGIN_MESSAGE_MAP(CD3DMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_MFCCOLORBUTTON1, &CD3DMFCDlg::OnBnClickedMfccolorbutton1)
	ON_BN_CLICKED(IDC_BUTTON1, &CD3DMFCDlg::OnBnClickedButton1)
	ON_WM_SIZE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


// CD3DMFCDlg 消息处理程序

BOOL CD3DMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	
	CRect rectDlg;
	GetClientRect(&rectDlg);
	app = new D3DApp(m_hWnd, rectDlg.Width(), rectDlg.Height(), _T("res\\shader.hlsl"));

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CD3DMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CD3DMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
		
	}
	else
	{
		CDialogEx::OnPaint();		
	}
	
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CD3DMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CD3DMFCDlg::OnBnClickedMfccolorbutton1()
{
	LoadLibrary(L"E:\\项目文件\\VS Workspace\\实验室\\Dx12HookExample-master\\Dx12HookExample\\x64\\Debug\\DX12HOOKEXAMPLE.dll");
	COLORREF cf = ((CMFCColorButton*)GetDlgItem(IDC_MFCCOLORBUTTON1))->GetColor();
}

void CD3DMFCDlg::OnBnClickedButton1()
{
	CFileDialog cf(TRUE, NULL, NULL, OFN_FILEMUSTEXIST, _T("图片/视频文件(*.png,*.gif,*.mp4)|*.png;*.gif;*.mp4||"), this);
	if (cf.DoModal() == 1) {
		app->loadtexture(cf.GetPathName().AllocSysString());
	}
}


void CD3DMFCDlg::OnSize(UINT nType, int cx, int cy)
{		
	CDialogEx::OnSize(nType, cx, cy);
	if(app!=nullptr)
		app->onReSize(cx,cy);
	// TODO: 在此处添加消息处理程序代码
}


void CD3DMFCDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMouseMove(nFlags, point);
	app->OnMouseMove(point.x, point.y);
}


void CD3DMFCDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonDown(nFlags, point);
	app->OnLeftMouseDown(point.x, point.y);
}


void CD3DMFCDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonUp(nFlags, point);
	app->OnLeftMouseUp(point.x, point.y);
}


void CD3DMFCDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnRButtonDown(nFlags, point);
	app->OnRightMouseDown(point.x, point.y);
}

void CD3DMFCDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnRButtonUp(nFlags, point);
	app->OnRightMouseUp(point.x, point.y);
}




BOOL CD3DMFCDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	
	if (zDelta == 120) {
		app->OnMouseWheel(0.5);
	}
	else if (zDelta == -120) {
		app->OnMouseWheel(-0.5);
	}
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

