#include <d3d12.h>//D3D12核心头文件
#include <dxgi.h>//dxgi是R3层渲染接口的底层
#include <DirectXMath.h>//使用D3D提供的数学算法
#include <d3dcompiler.h>//编译着色器所在头文件
#include <dxgidebug.h>//DXGI调试器
#include <d3d11on12.h>//D3D12需要使用D3D11库创建D2D文字以及DWrite
#include <d2d1_3.h>//2D绘制图形
#include <dwrite.h>//2D绘制文字
#include <strsafe.h>  //for StringCchxxxxx function
#include <wincodec.h>//Windows Image Component（Windows图像组件）图片解码库
#include <wrl.h>//调用WRL库（Windows Runtime Library）中的COM接口，为了使用COM智能指针
/*
	D3D12是使用COM接口（Component Object Model 组件对象模型）设计的
* 为了辅助用户管理COM对象的生命周期，我们可以把它当作是COM对象的智能指针。
当一个ComPtr实例超出作用域范围时，它便会自动调用相应COM对象的Release方法。
* 智能指针重载了->*等运算符，我们可以把智能指针当做C指针那样使用
*/
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace D2D1;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

class Vertex {//顶点类
public:
	XMFLOAT3 position;//XMFLOAT3也就是float[3]数组，3个代表X、Y、Z
	XMFLOAT4 color;//R、G、B、A
};

class MeshGeometry {
public:
	ComPtr<ID3DBlob> vertexBufferCPU, indexBufferCPU;
	ComPtr<ID3D12Resource> vertexUploadBuffer, indexUploadBuffer, vertexBufferGPU, indexBufferGPU;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	UINT indexCount;
};

class MVP
{
public:
	XMFLOAT4X4 mvp = //经典的Model View Projection(MVP)矩阵.
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
};

class MapControllerCamera {

};

class D3DApp{
public:
	HWND hWnd;
	UINT windowWidth;
	UINT windowHeight;
	WCHAR GPUcardName[128];

	FLOAT rtvBackgroundColorHSLA[4] = {//HSL值，Hue 色调Saturation 饱和度Luminance 亮度
		0.0f//R
		,0.0f//G
		,0.0f//B
		,0.0f//A
	};


	D3DApp(
		HWND hWnd//在哪个窗口上绘制？填入此窗口的句柄
		, UINT windowWidth//窗口宽度
		, UINT windowHeight//窗口高度
		, WCHAR* shaderFilePath//着色器文件名
		, BOOL Windowed=TRUE//TRUE窗口模式	false全屏模式
		,BOOL debug = TRUE//TRUE启用调试 false不启用
	);
	virtual ~D3DApp();


	void loadtexture(LPCWSTR texcuteFilePath);
	/*
		更新窗口大小很重要，这涉及到重置命令队列，遍历交换链缓冲区创建新的RTV
		更新深度DSV、过渡新的资源屏障等等，因此必须随窗口而适应
	*/
	void onReSize(UINT newWindowWidth, UINT newWindowHeight);
	void OnLeftMouseDown(int x, int y);
	void OnLeftMouseUp(int x, int y);
	void OnRightMouseDown(int x, int y);
	void OnRightMouseUp(int x, int y);
	void OnMouseMove(int x, int y);
	void OnMouseWheel(
		float step//推荐步长：±0.5		+0.5向前移动		-0.5向后移动
	);
	void render();//渲染
private:
	ComPtr<ID3D12Debug> debugController;//DX调试器对象
	ComPtr<IDXGIFactory1>dxgiFactory;//IDXGIFactory对象是DX中的工厂对象，提供各种函数
	ComPtr < ID3D12Device> d3dDevice;//ID3D12Device设备对象，设备用来创造其他对象
	/*
	* 每个GPU都至少维护着一个命令队列（command queue，本质上是环形缓冲区）。
	CPU可利用命令列表（command list）将命令提交到这个队列中去。当一系列命令被提交至命令队列之时，
	它们并不会被GPU立即执行。由于GPU可能正在处理先前插入命令队列内的命令，因此，
	后来新到的命令会一直在这个队列之中等待执行。
	还有一种与命令列表有关的名为ID3D12CommandAllocator的内存管理类接口。
	记录在命令列表内的命令实际上是存储在与之关联的命令分配器（command allocator）上
	当通过ID3D12CommandQueue::ExecuteCommandLists方法执行命令列表的时候，命令队列就会引用分配器里的命令。
	*/
	ComPtr < ID3D12CommandQueue> commandQueue;//命令队列对象
	ComPtr <ID3D12GraphicsCommandList> graphicsCommandList;//图形命令列表对象
	ComPtr <ID3D12CommandAllocator> commandAllocator;//命令分配器对象
	/*
	* 为了避免渲染中出现卡帧的现象，最好将一帧完整地绘制在一种称为后台缓冲区的离屏纹理内。
		当后台缓冲区中的动画帧绘制完成之后，后台缓冲区和前台缓冲区的角色互换：
		后台缓冲区变为前台缓冲区呈现新一帧的画面，
		而前台缓冲区则为了展示动画的下一帧转为后台缓冲区，等待填充数据。
		我们把交换链缓冲区中的离屏画面称之为渲染目标视图（RTV，但凡是这种堆、资源的数据一律由根签名绑定到渲染管线）
		前后台缓冲的这种互换操作称为呈现（presenting，提交）。呈现是一种高效的操作，
		只需交换指向当前前台缓冲区和后台缓冲区的两个指针即可实现。
		前台缓冲区和后台缓冲区互相交换显示构成了交换链（swap chain）。
		Direct3D中用IDXGISwapChain接口表示。这个接口不仅存储了前台缓冲区和后台缓冲区两种纹理，
		而且还提供了修改缓冲区大小（IDXGISwapChain::ResizeBuffers）和呈现缓冲区内容（IDXGISwapChain::Present）的方法
		使用两个缓冲区（前台和后台）的情况称为双缓冲（double buffering，也有叫双重缓冲、双倍缓冲）。
		也可以运用更多的缓冲区。例如，使用3个缓冲区就叫作三重缓冲（三倍缓冲等）
	*/
	ComPtr <IDXGISwapChain> swapChain;//交换链对象
	static const UINT swapChainBufferCount=2;//交换链缓冲区数量，交换链至少要有2个缓冲区，因为有2个才能互相交换
	UINT currentSwapChainBufferNO=0;//当前使用的交换链缓冲区序号
	ComPtr < ID3D12DescriptorHeap> rtvHeap;//RTV描述符堆		Render Target View（渲染目标视图）
	ComPtr < ID3D12Resource> swapChainBufferResource[swapChainBufferCount];//资源是D3D渲染数据格式，比如顶点数据、纹理数据
	UINT rtvHeapSize;
	/*
	* 假设场景中有一些不透明的物体，那么离摄像机最近的物体上的点便会遮挡住它后面一切物体上的对应点
	深度缓冲就是一种用于确定在场景中离摄像机最近点的技术。通过这种技术，我们就不必再担心场景中物体的绘制顺序了。
	*/
	ComPtr < ID3D12DescriptorHeap> dsvHeap;//DSV描述符堆		Depth Stencil View（深度缓冲视图）
	ComPtr < ID3D12Resource> dsvResource;
	/*
		常量缓冲区（constant buffer）：也是一种GPU资源（ID3D12Resource）
		其数据内容可供着色器程序所引用。通常由CPU每帧更新一次。
		举例：相机在场景中不断移动，那么GPU引用的资源就需要不断更新渲染
		这种资源就应为CBV资源，并且是上传堆upload buffer资源，也就是由CPU计算新的视图矩阵后重新上传至GPU的资源
	*/
	ComPtr < ID3D12DescriptorHeap> cbvHeap;//CBV描述符堆		Constant Buffer View（常量缓冲视图）
	ComPtr<ID3D12Resource> cbvResource;//CBV使用的上传堆
	ComPtr<ID3D12DescriptorHeap> srvHeap;//SRV描述符堆		Shader Resource View（着色器资源视图）
	
	/*
		根签名（root signature）：定义：在执行绘制命令之前，那些应用程序将绑定到渲染流水线上的资源
		它们会被映射到着色器的对应输入寄存器。
		如果把着色器程序当作一个函数，而将输入资源看作着色器的函数参数，那么根签名则定义了函数签名。
		从 PSO 被创建之时起，根签名和着色器程序的组合就开始生效
		D3D中，根签名由ID3D12RootSignature接口来表示，并以一组描述绘制调用过程中着色器所需资源的根参数（root parameter）定义而成。
		根参数可以是 根常量（root constant）、根描述符（root descriptor）、根描述符表（descriptor table）
		描述符表指定的是描述符堆中存有描述符的一块连续区域。
		在代码中要通过填写 CD3DX12_ROOT_PARAMETER 结构体来描述根参数
	*/
	ComPtr < ID3D12RootSignature> rootSignature;//根签名对象
	ComPtr<ID3DBlob> vsData, psData;//着色器数据，一个是VS（Vertex Shader）顶点着色器，一个是PS（Pixel Shader）像素着色器
	/*
		流水线状态对象（Pipeline State Object，PSO）：大多数控制图形流水线状态的对象被统称为流水线状态对象
		用 ID3D12PipelineState 接口表示。要创建PSO，我们首先要填写一份描述其细节的 D3D12_GRAPHICS_PIPELINE_STATE_DESC 结构体实例
		如果把一个PSO与命令列表相绑定，那么，在我们设置另一个 PSO 或重置命令列表之前，会一直沿用当前的 PSO 绘制物体。
	*/
	ComPtr < ID3D12PipelineState> pipelineState;//渲染管线状态对象
	/*
		例子：CPU向命令队列里添加绘制Cube位置信息P1的命令A1，由于向命令队列添加命令并不会阻塞CPU
		所以CPU会继续执行后序指令。在GPU执行绘制命令A1之前，如果CPU率先覆写了数据，提前把其中的位置信息修改为P2
		那么这个行为就会造成一个严重的错误。这时不管是按原命令绘制，还是在绘制的过程中更新资源都是错误的行为。
		解决此问题的一种办法是：强制CPU等待，直到GPU完成所有命令的处理，达到某个指定的围栏点（fence point）为止
		我们将这种方法称为刷新命令队列（flush command queue），可以通过围栏（fence）来实现这一点
		围栏用 ID3D12Fence 接口来表示，此技术能用于实现GPU和CPU间的同步。
		强制CPU等待直到GPU完成所有命令的处理，达到某个指定的围栏点fence point
	*/
	ComPtr<ID3D12Fence>					fence;//围栏
	UINT64 currentFenceValue=0;//围栏标记值
	void flushCommandQueue();//刷新命令队列
	
	void initd3dDevice();
	void initcommandQueue();
	void initcommandAllocator();
	void initgraphicsCommandList();
	void initdxgiSwapChain(BOOL Windowed);
	void initrtvHeap();
	void initdsvHeap();
	void initcbvHeap();
	void initsrvHeap();
	void initrootSignature();
	void compileSharder(WCHAR* shaderFilePath);
	void initpipelineState();
	void initfence();
	void initD2D();

	void draw();
	ComPtr < ID3D12Resource> createGeometryResource(UINT64 RowPitch, const void* pData, ComPtr<ID3D12Resource>& uploadBuffer);



	D3D12_DESCRIPTOR_HEAP_DESC createD3D12_DESCRIPTOR_HEAP_DESC(//创建描述符堆
		D3D12_DESCRIPTOR_HEAP_TYPE Type= D3D12_DESCRIPTOR_HEAP_TYPE_RTV//堆的类型
		,UINT NumDescriptors= swapChainBufferCount//堆中的描述符数量。如果是RTV堆，应设为交换链缓冲区数量
		,D3D12_DESCRIPTOR_HEAP_FLAGS Flags= D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		/*
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		*/
		,UINT NodeMask=0//对于单适配器操作，将此设置为0。如果有多个适配器节点，请设置一个位来标识描述符堆应用到的节点（设备的物理适配器之一）。掩码中的每一位对应一个节点。只需设置一位。请参阅多适配器系统。
	);

	/*
		堆（Heap）：GPU资源都存于堆中，其本质是具有特定属性的 GPU 显存块。
		为了使性能达到最佳，通常应将资源放置于默认堆中。只有在需要使用上传堆或回读堆的特性之时才选用其他类型的堆。
		对于静态几何体（static geometry，即每一帧都不会发生改变的几何体）而言
		我们会将其顶点缓冲置于默认堆（D3D12_HEAP_TYPE_DEFAULT）中来优化性能。
	*/
	D3D12_HEAP_PROPERTIES createD3D12_HEAP_PROPERTIES(//创建堆
		D3D12_HEAP_TYPE Type= D3D12_HEAP_TYPE_DEFAULT
		/*
			D3D12_HEAP_TYPE_DEFAULT		默认堆的资源只能由GPU访问
			D3D12_HEAP_TYPE_UPLOAD		由CPU上传到GPU的资源，常见的就是此资源在经过CPU的计算修改后，上传至GPU再绑定到渲染管线
			D3D12_HEAP_TYPE_READBACK	由GPU 读回数据进行优化的 CPU 访问，此堆中的资源必须使用D3D12_RESOURCE_STATE _COPY_DEST 创建，并且无法更改。
			D3D12_HEAP_TYPE_CUSTOM		指定自定义堆
		*/
		,D3D12_CPU_PAGE_PROPERTY CPUPageProperty= D3D12_CPU_PAGE_PROPERTY_UNKNOWN
		,D3D12_MEMORY_POOL MemoryPoolPreference= D3D12_MEMORY_POOL_UNKNOWN
		,UINT CreationNodeMask=1
		,UINT VisibleNodeMask=1
	);

	D3D12_RESOURCE_DESC createD3D12_RESOURCE_DESC(//创建资源
		UINT64 Width
		,UINT Height
		,D3D12_RESOURCE_DIMENSION Dimension= D3D12_RESOURCE_DIMENSION_UNKNOWN
		/*
			D3D12_RESOURCE_DIMENSION_UNKNOWN 资源类型未知。
			D3D12_RESOURCE_DIMENSION_BUFFER 资源是一个缓冲区。
			D3D12_RESOURCE_DIMENSION_TEXTURE1D 资源是一维纹理。
			D3D12_RESOURCE_DIMENSION_TEXTURE2D 资源是 2D 纹理。
			D3D12_RESOURCE_DIMENSION_TEXTURE3D 资源是 3D 纹理。
		*/
		,UINT64 Alignment=0
		,UINT16 DepthOrArraySize=1//指定资源的深度（如果是 3D）或数组大小（如果它是 1D 或 2D 资源的数组）。
		,UINT16 MipLevels=1//指定 MIP 级别的数量。
		,DXGI_FORMAT Format= DXGI_FORMAT_UNKNOWN
		, UINT Count=1
		,UINT Quality=0
		,D3D12_TEXTURE_LAYOUT Layout= D3D12_TEXTURE_LAYOUT_UNKNOWN//表示纹理数据以行优先顺序（有时称为“间距线性顺序”）存储。
		/*
			D3D12_TEXTURE_LAYOUT_UNKNOWN	= 0,
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR	= 1,
        D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE	= 2,
        D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE	= 3
		*/
		,D3D12_RESOURCE_FLAGS Flags= D3D12_RESOURCE_FLAG_NONE
		/*
			D3D12_RESOURCE_FLAG_NONE	= 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET	= 0x1,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL	= 0x2,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS	= 0x4,
        D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE	= 0x8,
        D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER	= 0x10,
        D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS	= 0x20,
        D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY	= 0x40,
        D3D12_RESOURCE_FLAG_VIDEO_ENCODE_REFERENCE_ONLY	= 0x80,
        D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE	= 0x100
		*/
	);

	/*
	* 为了实现常见的渲染效果，GPU通常会在一个步骤中写入资源R，然后在后面的步骤中读取资源R。但是
	如果GPU没有完成对资源的写入，或者根本没有开始写入，那么从资源读取将是一个资源风险。为了解决这个问题
	Direct3D将一个状态关联到资源。资源在创建时处于默认状态，这取决于应用程序告诉Direct3D任何状态转换
	这使GPU能够做任何它需要做的工作，以完成过渡和防止资源危害。例如，如果我们写入一个资源，比如一个纹理
	我们将纹理状态设置为渲染目标状态;当我们需要读取纹理时，我们将其状态更改为着色器资源状态。通过通知Direct3D转换
	GPU可以采取一些措施来避免风险，例如等待所有的写操作完成后再从资源中读取。由于性能原因
	资源转换的负担落在了应用程序开发人员身上。应用程序开发人员知道这些转换何时发生。自动过渡跟踪系统会增加额外的开销。
	* 转换资源屏障（transition resource barrier）：通过命令列表设置转换资源屏障数组
		即可指定资源的转换（比如从渲染目标状态转换为着色器资源状态）
	*/
	D3D12_RESOURCE_BARRIER createD3D12_RESOURCE_BARRIER(//创建资源屏障
		ID3D12Resource * pResource
		,D3D12_RESOURCE_STATES StateBefore
		,D3D12_RESOURCE_STATES StateAfter
		,D3D12_RESOURCE_BARRIER_TYPE Type= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION
		,D3D12_RESOURCE_BARRIER_FLAGS Flags= D3D12_RESOURCE_BARRIER_FLAG_NONE
		,UINT Subresource= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
	);

	D3D12_CPU_DESCRIPTOR_HANDLE getD3D12_CPU_DESCRIPTOR_HANDLE() {
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapCPUHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHeapCPUHandle.ptr += (currentSwapChainBufferNO * rtvHeapSize);
		//注意这里移动指针并不是到下一个RTV	而是从RTV的堆开始到当前缓冲区的偏移
		return rtvHeapCPUHandle;
	}
	
	//初始的默认摄像机的位置
	XMFLOAT4X4 mWorld =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	XMFLOAT4X4 mView =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	XMFLOAT4X4 mProj =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	D3D12_VIEWPORT viewPort = { 0.0f, 0.0f,
		static_cast<FLOAT>(windowWidth),
		static_cast<FLOAT>(windowHeight),
		D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };//视图端口对象
	D3D12_RECT	 rect = { 0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
	POINT lastRecordMousePosition;
	bool isLeftDown = false;
	bool isRightDown = false;
	BYTE* mMappedData;
	

	//在这里 Vector Position和Point是一个意思
	XMVECTOR cameraPosition = XMVectorSet(0.f, 0.f, -10.f, 1.0f);
	XMVECTOR lookAtPoint = XMVectorZero();//焦点
	float angle = 180.f;//cameraPosition相对lookAtPoint的角度
	float radian= XM_PI;

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;
	

	float get_cameraPosition_lookAtPoint_between_StraightDistance();//计算得到cameraPosition与lookAtPoint之间的3D直线距离
	
	MeshGeometry cube;
	D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }//此输入布局告诉IA，顶点缓冲区中的每个顶点都有一个元素，该元素应绑定到顶点着色器中的“POSITION”参数。它还说该元素从顶点的第一个字节（第二个参数为 0）开始，包含 3 个浮点数，每个浮点数为 32 位或 4 个字节（第三个参数，DXGI_FORMAT_R32G32B32_FLOAT）
		,{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		//上一个 POSITION 元素占12 个字节，所以我们需要从 POSITION 之后，也就是从第12个字节开始插入 COLOR，这就是为什么第五个参数是 12。
	};


	

	ComPtr<ID3D11Device>			d3d11Device;
	ComPtr<ID3D11DeviceContext>		d3d11DeviceContext;
	ComPtr<ID3D11On12Device>			d3d11On12Device;
	ComPtr<IDXGIDevice> dxgiDevice;
	ComPtr<ID2D1DeviceContext>			d2dDeviceContext;
	ComPtr<ID2D1Device>				d2dDevice;
	ComPtr<IDWriteFactory>				dWriteFactory;
	ComPtr<ID3D11Resource>				d3d11WrappedResource[swapChainBufferCount];
	ComPtr<ID2D1Bitmap1>				d2dBitmap[swapChainBufferCount];
	ComPtr<ID2D1SolidColorBrush>		d2dSolidColorBrush;
	ComPtr<IDWriteTextFormat>			writeTextFormat;
	WCHAR D2DString[2000]=
		L"显卡名称：%ls"
		L"\n相机自身坐标（ % .2f, % .2f, % .2f, % .2f ）"
		L"\n相机焦点坐标（ % .2f, % .2f, % .2f, % .2f ）"
		L"\n相机与焦点之间的直线距离 % .4f"
		L"\nobjConstants"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\nworld"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\nmView"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\nmProj"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
		L"\n[% .2f, % .2f, % .2f, % .2f]"
	;




	ComPtr<ID3D12DescriptorHeap>		pISamplerDescriptorHeap;
	ComPtr<IWICImagingFactory>			wicFactory;//wic工厂
	ComPtr<IWICBitmapDecoder>			wicDecoder;//解码器对象
	ComPtr<IWICBitmapSource>			bmpSource;
	UINT bmpRowByte;
	ComPtr<ID3D12Heap>					pITextureHeap;
	ComPtr<ID3D12Heap>					pIUploadHeap;
	ComPtr<ID3D12Resource>				pITexture;
	ComPtr<ID3D12Resource>				pITextureUpload;
	UINT64 n64UploadBufferSize;
};