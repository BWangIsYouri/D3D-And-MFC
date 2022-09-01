#include "D3DApp.h"

void D3DApp::initd3dDevice()
{
	CreateDXGIFactory1(//在COM中，有一个创建COM对象的约定，就是传入IID（Interface ID）得到期望的接口类型
		IID_PPV_ARGS(dxgiFactory.GetAddressOf())
		/*
		*	这个宏的具体如下：
		*	__uuidof(&dxgiFactory),//__uuidof运算符获取类型值的ID，从而得到IID
			reinterpret_cast<void**>(&dxgiFactory)//调用IID_PPV_ARGS_Helper()函数进行参数检查
			__uuidof(**(ppType))将获取(**(ppType))的COM接口ID（Globally Unique IDentifier，全局唯一标识符，GUID）
			IID_PPV_ARGS宏的本质是将ppType强制转换为void**类型。我们在很多地方都会见到此宏的身影
			这是因为在调用Direct3D 12中创建接口实例的API时，大多都有一个参数是类型为void**的待创接口COM ID。
		*/
	);
	/*
		其实IXXX3继承IXXX2，IXXX2继承IXXX1如果你很奇怪接口和函数名后面的数字
		那么你现在就理解他们为对应接口或函数的版本号
		默认为0，也就是第一个原始版本，不用写出来，2就表示升级的第三个版本，依此类推 
	*/
	dxgiFactory->MakeWindowAssociation(
		hWnd,
		DXGI_MWA_NO_ALT_ENTER//关闭ALT+ENTER键切换全屏的功能
	);
	/*
	* 显示适配器（display adapter）是一种硬件设备（例如独立显卡）
	但某些软件显示适配器能够模拟硬件的图形处理功能。一个系统中可能会存在数个适配器（比如装有数块显卡）
	*/
	ComPtr< IDXGIAdapter1>dxgiAdapter;//适配器
	DXGI_ADAPTER_DESC1 stAdapterDesc = {};
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(i, &dxgiAdapter); ++i)
	{//寻找当前设备上所有的枚举适配器
		dxgiAdapter->GetDesc1(&stAdapterDesc);//获取有关该适配器的信息描述
		if (stAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)//跳过软件虚拟适配器设备
			continue;
		if (
			SUCCEEDED(//试试能否在此适配器上创建设备
				D3D12CreateDevice(
					dxgiAdapter.Get(),//使用合适的适配器创建设备，该参数可为null,表示使用默认的适配器
					D3D_FEATURE_LEVEL_12_1,//DX版本
					IID_PPV_ARGS(d3dDevice.GetAddressOf())
				)
			)
		){
			for (int i = 0; i < _countof(GPUcardName); i++)
				GPUcardName[i] = stAdapterDesc.Description[i];
			break;
		}
	}
}

void D3DApp::initcommandQueue() 
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//0,指定 GPU 可以执行的命令缓冲区。直接命令列表不继承任何 GPU 状态。
	/*
        D3D12_COMMAND_LIST_TYPE_BUNDLE	= 1,指定只能通过直接命令列表直接执行的命令缓冲区。捆绑命令列表继承所有 GPU 状态（当前设置的管道状态对象和原始拓扑除外）。
        D3D12_COMMAND_LIST_TYPE_COMPUTE	= 2,指定用于计算的命令缓冲区。
        D3D12_COMMAND_LIST_TYPE_COPY	= 3,指定用于复制的命令缓冲区。
        D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE	= 4,指定用于视频解码的命令缓冲区。
        D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS	= 5,指定用于视频处理的命令缓冲区。
        D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE	= 6 指定用于视频编码的命令缓冲区。
	*/
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//命令队列的优先级。
	/*
	* D3D12_COMMAND_QUEUE_PRIORITY_NORMAL = 0,正常优先级。
	  D3D12_COMMAND_QUEUE_PRIORITY_HIGH = 100,高优先级。
	  D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME = 10000 全局实时优先级。
	*/
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//设为 D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT 表示应该为此命令队列禁用 GPU 超时。
	queueDesc.NodeMask =0;//对于单 GPU 操作，将其设置为零。
	d3dDevice->CreateCommandQueue(
		&queueDesc,
		IID_PPV_ARGS(commandQueue.GetAddressOf())
	);
}

void D3DApp::initcommandAllocator() 
{
	d3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(commandAllocator.GetAddressOf())
	);
}

void D3DApp::initgraphicsCommandList() 
{
	d3dDevice->CreateCommandList(
		0,//对于单 GPU 操作，将其设置为0。如果有多个 GPU 节点，则设置一个位来标识要为其创建命令列表的节点（设备的物理适配器）。掩码中的每一位对应一个节点。只需设置一位。
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(),//指向设备创建命令列表的命令分配器对象的指针。
		nullptr,//pipelineState.Get(),//指向管道状态对象的可选指针，其中包含命令列表的初始管道状态。如果是nullptr，则运行时设置一个虚拟的初始管道状态，以便驱动程序不必处理未定义的状态。这样做的开销很低，特别是对于命令列表，记录命令列表的总成本可能会使单个初始状态设置的成本相形见绌。因此，如果这样做不方便，不设置初始管道状态参数几乎没有成本。另一方面，对于捆绑包，尝试设置初始状态参数可能更有意义（因为捆绑包总体上可能更小，并且可以经常重用）。
		IID_PPV_ARGS(graphicsCommandList.GetAddressOf())
	);
	graphicsCommandList->Close();
}

void D3DApp::initdxgiSwapChain(BOOL Windowed)
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc={};
	swapChainDesc.BufferDesc.Width = windowWidth;//缓冲区宽度
	swapChainDesc.BufferDesc.Height = windowHeight;//缓冲区高度
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;//帧率分子
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;//帧率分母，这样设置也就是一分之六十的帧率
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//每个元素由4个8位无符号分量构成，每个分量都被映射到 [0, 1] 区间。
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;//扫描线顺序未指定。
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;//未指定的缩放。
	/*
		DXGI_MODE_SCALING_CENTERED指定不缩放。图像位于显示屏的中心。该标志通常用于固定点间距显示器（例如 LED 显示器）。
		DXGI_MODE_SCALING_STRETCHED指定拉伸缩放。
	*/
	swapChainDesc.SampleDesc.Count = 1;//每个像素的多重采样数。
	swapChainDesc.SampleDesc.Quality = 0;//图像质量水平。质量越高，性能越低
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//使用表面或资源作为输出渲染目标。
	swapChainDesc.BufferCount = swapChainBufferCount;//交换链后台缓冲区数量
	swapChainDesc.OutputWindow = hWnd;//缓冲区输出窗口句柄
	swapChainDesc.Windowed = Windowed;//窗口模式，false为全屏模式
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//使用此标志指定翻转表示模型，并指定 DXGI 在调用IDXGISwapChain::Present()后丢弃后台缓冲区的内容。此标志不能与多重采样和部分表示一起使用。
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	/*
	*	DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH 设置此标志以使应用程序能够通过调用IDXGISwapChain::ResizeTarget来切换模式。从窗口模式切换到全屏模式时，显示模式（或显示器分辨率）将更改以匹配应用程序窗口的尺寸。
		DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY 
		MSDN介绍：
			设置此标志以将呈现的内容限制为本地显示。因此，无法通过远程访问或桌面复制 API访问呈现的内容。
			此标志支持 Windows 的窗口内容保护功能。应用程序可以使用此标志来保护自己的屏幕窗口内容不被通过一组特定的公共操作系统功能和 API 捕获或复制。
			如果将此标志与另一个进程创建HWND的窗口化（ HWND或IWindow ）交换链一起使用，则HWND的所有者必须使用 SetWindowDisplayAffinity适当地运行以允许对IDXGISwapChain::Present或IDXGISwapChain1::Present1的调用成功。
		我的介绍：
			可曾用过一些直播软件或录屏软件？这些录屏软件能否控制录制显示的屏幕画面内容？
			例如一些游戏外挂主播使用这种软件仅向观众展示游戏画面而隐藏外挂显示的画面，这种底层技术就是这样实现的
			设置这个标志就意味着录屏或直播软件通过网络或远程API录制的图像无法显示此交换链缓冲区所呈现IDXGISwapChain::Present()的画面
	*/
	dxgiFactory->CreateSwapChain(
		commandQueue.Get()//当交换链对象调用Present()方法时，由这个命令队列记录提交画面的命令
		,&swapChainDesc
		,swapChain.GetAddressOf()
	);
	dxgiFactory.Reset();//交换链创建完毕，工厂就没用了，释放内存
}

void D3DApp::initrtvHeap() 
{
	d3dDevice->CreateDescriptorHeap(
		&createD3D12_DESCRIPTOR_HEAP_DESC(),
		IID_PPV_ARGS(rtvHeap.GetAddressOf())
	);
}

void D3DApp::initdsvHeap()
{
	d3dDevice->CreateDescriptorHeap(
		&createD3D12_DESCRIPTOR_HEAP_DESC(D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1)
		,IID_PPV_ARGS(dsvHeap.GetAddressOf())
	);
}

void D3DApp::initcbvHeap() 
{
	d3dDevice->CreateDescriptorHeap(
		&createD3D12_DESCRIPTOR_HEAP_DESC(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
		IID_PPV_ARGS(&cbvHeap)
	);
	/*
		常量缓冲区必须是最小硬件的倍数，分配大小(通常为256字节)。四舍五入到最近256的倍数
		我们通过添加255然后屏蔽来做到这一点，存储所有< 256位的下2个字节。
		//示例:设byteSize = 300。
		// (300 + 255) & ~255
		// 555 & ~255
		// 0x022B & ~0x00ff
		// 0x022B & 0xff00
		/ / 0 x0200
		/ / 512
	*/
	d3dDevice->CreateCommittedResource(
		&createD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&createD3D12_RESOURCE_DESC((sizeof(MVP) + 255) & ~255,1, D3D12_RESOURCE_DIMENSION_BUFFER,0,1,1, DXGI_FORMAT_UNKNOWN,1,0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cbvResource)
	);
	cbvResource->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData));
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbvResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (sizeof(MVP) + 255) & ~255;
	d3dDevice->CreateConstantBufferView(
		&cbvDesc,
		cbvHeap->GetCPUDescriptorHandleForHeapStart()
	);
}

void D3DApp::initsrvHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
	stSRVHeapDesc.NumDescriptors = 2; //我们将纹理视图描述符和CBV描述符放在同一个描述符堆，也就是2个
	stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDevice->CreateDescriptorHeap(&stSRVHeapDesc, IID_PPV_ARGS(&srvHeap));
	D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
	stSamplerHeapDesc.NumDescriptors = 5;//创建5个采样器
	stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDevice->CreateDescriptorHeap(&stSamplerHeapDesc, IID_PPV_ARGS(&pISamplerDescriptorHeap));
}

void D3DApp::initrootSignature() 
{
	D3D12_ROOT_PARAMETER1 stRootParameters[1] = {};
	stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//指定根签名槽的类型
	/*
			D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE = 0,该槽用于描述符表。
			D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,该插槽用于根常量。
			D3D12_ROOT_PARAMETER_TYPE_CBV,该插槽用于恒定缓冲区视图 (CBV)。
			D3D12_ROOT_PARAMETER_TYPE_SRV,该插槽用于着色器资源视图 (SRV)。
			D3D12_ROOT_PARAMETER_TYPE_UAV 该插槽用于无序访问视图 (UAV)。
	*/
	stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;//表布局中的描述符范围数。
	D3D12_DESCRIPTOR_RANGE1 cbvTable;
	cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//指定一系列常量缓冲区视图 (CBV)。
	/*
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV = 0,指定 SRV 的范围。
		  D3D12_DESCRIPTOR_RANGE_TYPE_UAV,指定一系列无序访问视图 (UAV)。
		  D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER 指定采样器的范围。
	*/
	cbvTable.NumDescriptors = 1;//范围内的描述符数。使用 -1 或 UINT_MAX 指定无限大小。如果给定的描述符范围是无界的，那么它必须是表定义中的最后一个范围，或者表定义中的以下范围必须具有一个不是D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND的OffsetInDescriptorsFromTableStart值。
	cbvTable.BaseShaderRegister = 0;//范围内的基本着色器寄存器。例如，对于着色器资源视图 (SRV)，3 映射到“: register(t3);” 在 HLSL 中。
	cbvTable.RegisterSpace = 0;//寄存器空间。通常可以为 0，但允许多个未知大小的描述符数组看起来不重叠。例如，对于 SRV，通过扩展BaseShaderRegister成员描述中的示例，5 映射到“: register(t3,space5);” 在 HLSL 中。
	cbvTable.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	/*
		D3D12_DESCRIPTOR_RANGE_FLAG_NONE = 0,描述符是静态的
		  D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE = 0x1,
		  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE = 0x2,
		  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE = 0x4,
		  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC = 0x8,
		  D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS = 0x10000
	*/
	cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//描述符中的偏移量，从被设置为此参数槽的根参数值的描述符表开始。D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND表示该范围应紧跟前面的范围。
	stRootParameters[0].DescriptorTable.pDescriptorRanges = &cbvTable;//pDescriptorRanges是const成员，必须在初始化之前
	/*stRootParameters[0].Constants.ShaderRegister = 0;
	stRootParameters[0].Constants.RegisterSpace = 0;
	stRootParameters[0].Constants.Num32BitValues = 0;
	stRootParameters[0].Descriptor.ShaderRegister = 0;
	stRootParameters[0].Descriptor.RegisterSpace = 0;
	stRootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	D3D12_ROOT_DESCRIPTOR_FLAG_NONE	= 0,
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE	= 0x2,
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE	= 0x4,
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC	= 0x8
	*/
	stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	/*
		D3D12_SHADER_VISIBILITY_ALL = 0,指定所有着色器阶段都可以访问根签名槽处绑定的任何内容。
		  D3D12_SHADER_VISIBILITY_VERTEX = 1,指定顶点着色器阶段可以访问绑定在根签名槽的任何内容。
		  D3D12_SHADER_VISIBILITY_HULL = 2,指定外壳着色器阶段可以访问绑定在根签名槽的任何内容。
		  D3D12_SHADER_VISIBILITY_DOMAIN = 3,指定域着色器阶段可以访问绑定在根签名槽的任何内容。
		  D3D12_SHADER_VISIBILITY_GEOMETRY = 4,指定几何着色器阶段可以访问绑定在根签名槽的任何内容。
		  D3D12_SHADER_VISIBILITY_PIXEL = 5,指定像素着色器阶段可以访问根签名槽处绑定的任何内容。
		  D3D12_SHADER_VISIBILITY_AMPLIFICATION = 6,指定放大着色器阶段可以访问绑定在根签名槽的任何内容。
		  D3D12_SHADER_VISIBILITY_MESH = 7 指定网格着色器阶段可以访问根签名槽处绑定的任何内容。
	*/
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
	stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);//根签名中的槽数。这个数字也是pParameters数组中元素的数量。
	stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
	stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;//使用默认静态采样器的数量
	stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr; 
	stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ComPtr<ID3DBlob>pISignatureBlob,pIErrorBlob;
	D3D12SerializeVersionedRootSignature(
		&stRootSignatureDesc
		, pISignatureBlob.GetAddressOf()
		, pIErrorBlob.GetAddressOf()
	);
	d3dDevice->CreateRootSignature(
		0//对于单 GPU 操作，将其设置为零。
		, pISignatureBlob->GetBufferPointer()
		, pISignatureBlob->GetBufferSize()
		, IID_PPV_ARGS(&rootSignature)
	);
}

void D3DApp::initpipelineState()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	RtlZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = {vsData->GetBufferPointer(),vsData->GetBufferSize()};
	psoDesc.PS = {psData->GetBufferPointer(),psData->GetBufferSize()};
	psoDesc.BlendState.AlphaToCoverageEnable = true;// 指定在将像素设置为渲染目标时是否使用 alpha-to-coverage 作为多重采样技术
	psoDesc.BlendState.IndependentBlendEnable = true;//指定是否在同时渲染目标中启用独立混合。设置为TRUE以启用独立混合。如果设置为FALSE，则仅使用RenderTarget [0] 成员；RenderTarget [1..7] 被忽略。	
	for (auto& i : psoDesc.BlendState.RenderTarget) {
		i = {
			true,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,//允许将数据存储在所有组件中。
		};
	}
	psoDesc.SampleMask = UINT_MAX;//混合状态的示例蒙版。
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	/*
			D3D12_FILL_MODE_WIREFRAME	= 2,空心不填充
			D3D12_FILL_MODE_SOLID	= 3 实心填充
	*/
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;//不要绘制背面的三角形。
	/*
	*	D3D12_CULL_MODE_NONE 始终绘制所有三角形。
		D3D12_CULL_MODE_FRONT 不要绘制正面的三角形。
	*/
	psoDesc.RasterizerState.FrontCounterClockwise = false;//确定三角形是正面还是背面。如果此成员为TRUE，则​​如果三角形的顶点在渲染目标上为逆时针方向，则三角形将被视为正面，如果它们是顺时针方向，则三角形将被视为背面。如果此参数为FALSE，则相反。
	psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;//添加到给定像素的深度值
	psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;//像素的最大深度偏差。
	psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;//给定像素斜率上的标量。
	psoDesc.RasterizerState.DepthClipEnable = TRUE;//指定是否启用基于距离的剪裁。
	psoDesc.RasterizerState.MultisampleEnable = FALSE;//指定是否在多重采样抗锯齿 (MSAA) 渲染目标上使用四边形或 alpha 线抗锯齿算法。设置为TRUE以使用四边形线抗锯齿算法，设置为FALSE以使用 alpha 线抗锯齿算法。
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;//指定是否启用线条抗锯齿；仅适用于绘制线条且MultisampleEnable为FALSE的情况。
	psoDesc.RasterizerState.ForcedSampleCount = 0;//UAV 渲染或光栅化时强制的样本计数。有效值为 0、1、2、4、8 和可选的 16。0 表示不强制采样计数
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;//保守光栅化关闭。D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON开启保守光栅化。
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
	psoDesc.DepthStencilState.BackFace = defaultStencilOp;
	psoDesc.DepthStencilState.DepthEnable = TRUE;//是否启用深度缓冲
	psoDesc.DepthStencilState.StencilEnable = FALSE;//是否启用模板
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
	psoDesc.PrimitiveTopologyType =  D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//将输入图元解释为三角形。
	/*
	*	D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED着色器尚未使用输入基元类型初始化。
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT将输入原语解释为一个点。
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE将输入原语解释为一条线。
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH将输入图元解释为控制点补丁。
	*/
	psoDesc.NumRenderTargets = 1;//RTVFormats成员中呈现目标格式的 数量。
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	d3dDevice->CreateGraphicsPipelineState(
		&psoDesc,
		IID_PPV_ARGS(&pipelineState)
	);
}

void D3DApp::initfence()
{
	d3dDevice->CreateFence(
		0, 
		D3D12_FENCE_FLAG_NONE, 
		IID_PPV_ARGS(&fence)
	);
}

void D3DApp::initD2D()
{
	D2D1_FACTORY_OPTIONS dfp;
	dfp.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	ComPtr<ID2D1Factory7>				d2dFactory;
	D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED//工厂的线程模型和它创建的资源。
		/*
				D2D1_FACTORY_TYPE_SINGLE_THREADED = 0,不为访问或写入工厂或其创建的对象提供同步。如果从多个线程调用工厂或对象，则由应用程序提供访问锁定。
  D2D1_FACTORY_TYPE_MULTI_THREADED = 1,Direct2D 为访问和写入工厂及其创建的对象提供同步，从而能够从多个线程进行安全访问。
  D2D1_FACTORY_TYPE_FORCE_DWORD = 0xffffffff
		*/
		, __uuidof(ID2D1Factory7)
		, &dfp//提供给调试层的详细程度。
		, &d2dFactory
	);
	D3D11On12CreateDevice(//在 Direct3D 12 中创建使用 Direct3D 11 功能的设备，指定预先存在的 Direct3D 12 设备以用于 Direct3D 11 互操作。
		d3dDevice.Get()//指定用于 Direct3D 11 互操作的预先存在的 Direct3D 12 设备。不能为 NULL。
		, D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG
		/*
				D3D11_CREATE_DEVICE_SINGLETHREADED = 0x1,如果您的应用程序仅从单个线程调用 Direct3D 11 接口的方法，请使用此标志。默认情况下，ID3D11Device对象是 线程安全的。通过使用此标志，您可以提高性能。但是，如果您使用此标志并且您的应用程序从多个线程调用 Direct3D 11 接口的方法，则可能会导致未定义的行为。
  D3D11_CREATE_DEVICE_DEBUG = 0x2,创建一个支持调试层的设备。
  D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS = 0x8,防止创建多个线程。当此标志与Windows 高级光栅化平台 (WARP)设备一起使用时，WARP 不会创建其他线程，并且所有光栅化都将在调用线程上发生。不建议将此标志用于一般用途。见备注。
  D3D11_CREATE_DEVICE_BGRA_SUPPORT = 0x20,创建支持 BGRA 格式（DXGI_FORMAT_B8G8R8A8_UNORM和DXGI_FORMAT_B8G8R8A8_UNORM_SRGB）的设备。所有带有 WDDM 1.1+ 驱动程序的 10level9 和更高版本的硬件都支持 BGRA 格式。
  D3D11_CREATE_DEVICE_DEBUGGABLE = 0x40,使设备和驱动程序保留可用于着色器调试的信息。此标志的确切影响因驾驶员而异。
  D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT = 0x100,如果设备将产生需要两秒以上才能完成的 GPU 工作负载，并且您希望操作系统允许它们成功完成，则使用此标志。如果没有设置这个标志，操作系统会执行超时检测和恢复当它检测到执行时间超过两秒的 GPU 数据包时。如果设置了这个标志，操作系统允许这样一个长时间运行的数据包在不重置 GPU 的情况下执行。如果您的设备需要高度响应，我们建议不要设置此标志，以便操作系统可以检测并从 GPU 超时中恢复。如果您的设备需要执行耗时的后台任务（例如计算、图像识别和视频编码）以允许此类任务成功完成，我们建议设置此标志。
  D3D11_CREATE_DEVICE_VIDEO_SUPPORT = 0x800
		*/
		, nullptr//小于或等于 Direct3D 12 设备功能级别的第一个功能级别将用于执行 Direct3D 11 验证。如果未提供可接受的功能级别，则创建将失败。提供 NULL 将默认为 Direct3D 12 设备的功能级别。
		, 0//pFeatureLevels数组的大小（即元素的数量） 。
		, reinterpret_cast<IUnknown**>(commandQueue.GetAddressOf())//供 D3D11On12 使用的唯一队列数组。队列必须是 3D 命令队列类型。
		, 1//ppCommandQueues数组的大小（即元素的数量） 。
		, 0//使用 Direct3D 12 设备的哪个节点。只能设置 1 位。
		, &d3d11Device//输出参数
		, &d3d11DeviceContext//输出参数
		, nullptr//输出参数
	);
	d3d11Device.As(//COM智能指针,和QueryInterface()一样,同过D3D设备获取DXGI设备
		&d3d11On12Device//例如这里通过d3d11Device获得d3d11On12Device
	);
	d3d11On12Device.As(&dxgiDevice);
	d2dFactory->CreateDevice(//从给定的 IDXGIDevice 创建一个新的 Direct2D 设备
		dxgiDevice.Get()
		, &d2dDevice
	);
	d2dDevice->CreateDeviceContext(//创建设备上下文
		D2D1_DEVICE_CONTEXT_OPTIONS_NONE
		/*
				D2D1_DEVICE_CONTEXT_OPTIONS_NONE = 0,使用默认选项创建设备上下文。
  D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS = 1,跨多个线程分配渲染工作。
  D2D1_DEVICE_CONTEXT_OPTIONS_FORCE_DWORD = 0xffffffff
		*/
		, &d2dDeviceContext
	);
	d2dDeviceContext->CreateSolidColorBrush(
		ColorF(1.0f, 0.0f, 0.0f, 1.0f)//RGBA HSL值
		, &d2dSolidColorBrush
	);
	DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED
		/*
			DWRITE_FACTORY_TYPE_SHARED,指示 DirectWrite 工厂是一个共享工厂，它允许跨多个进程内组件重用缓存的字体数据。这样的工厂还利用跨进程字体缓存组件来获得更好的性能
  DWRITE_FACTORY_TYPE_ISOLATED 指示 DirectWrite 工厂对象是隔离的。从隔离工厂创建的对象不与来自其他组件的内部 DirectWrite 状态交互。
		*/
		, __uuidof(IDWriteFactory)
		, &dWriteFactory
	);
	dWriteFactory->CreateTextFormat(//创建字体
		L"微软雅黑",
		NULL,//指向字体集合对象的指针。当它为NULL时，表示系统字体集合。
		DWRITE_FONT_WEIGHT_NORMAL,//字体粗细。
		DWRITE_FONT_STYLE_NORMAL,//字体样式。
		DWRITE_FONT_STRETCH_NORMAL,//字体拉伸。
		20,//字体大小
		L"zh-cn", //中文字库
		&writeTextFormat
	);
	writeTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_JUSTIFIED);// 水平方向左对齐
	writeTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);//垂直方向居中
}

void D3DApp::loadtexture(LPCWSTR texcuteFilePath)
{
	wicFactory->CreateDecoderFromFilename(//创建纹理文件解码器
		texcuteFilePath,					// 文件路径
		nullptr,                            // 使用默认解码器
		GENERIC_READ,                    // 访问权限只读
		WICDecodeMetadataCacheOnDemand,  // 若需要就缓冲数据 
		&wicDecoder                    // 解码器对象
	);
	ComPtr<IWICBitmapFrameDecode>		pIWICFrame;
	wicDecoder->GetFrame(//获取帧
		0, // 要获取帧的索引，0表示获取第一帧图片(因为GIF。视频等格式文件可能会有多帧)
		&pIWICFrame
	);
	WICPixelFormatGUID wpf = {};
	pIWICFrame->GetPixelFormat(&wpf);//获取WIC图片格式
	WICPixelFormatGUID bmpFormat[] = {
		GUID_WICPixelFormat24bppBGR,//PNG	
		GUID_WICPixelFormat8bppIndexed//GIF
	};
	for (auto& i : bmpFormat) {
		if (InlineIsEqualGUID(i,wpf)) {
			if (!InlineIsEqualGUID(wpf, GUID_WICPixelFormat32bppRGBA)) {//如果原WIC格式不是直接能转换为DXGI格式的图片时，则转换图片格式为能够直接对应DXGI格式的形式
				ComPtr<IWICFormatConverter> pIConverter;//创建图片格式转换器
				wicFactory->CreateFormatConverter(&pIConverter);
				pIConverter->Initialize(//初始化一个图片转换器，实际也就是将图片数据进行了格式转换
					pIWICFrame.Get(),                // 输入原图片数据
					GUID_WICPixelFormat32bppRGBA,						 // 指定待转换的目标格式
					WICBitmapDitherTypeNone,         // 指定位图是否有调色板，现代都是真彩位图，不用调色板，所以为None
					nullptr,                            // 指定调色板指针
					0.0f,                             // 指定Alpha阀值
					WICBitmapPaletteTypeCustom       // 调色板类型，实际没有使用，所以指定为Custom
				);
				pIConverter.As(&bmpSource);// 调用QueryInterface方法获得对象的位图数据源接口
			}
			else
				pIWICFrame.As(&bmpSource);//图片数据格式不需要转换，直接获取其位图数据源接口
			UINT bmpWidth, bmpHeight;
			bmpSource->GetSize(&bmpWidth, &bmpHeight);//获得图片大小（单位：像素）
			ComPtr<IWICComponentInfo> pIWICmntinfo;
			wicFactory->CreateComponentInfo(GUID_WICPixelFormat32bppRGBA, pIWICmntinfo.GetAddressOf());
			//WICComponentType type;
			//pIWICmntinfo->GetComponentType(&type);
			ComPtr<IWICPixelFormatInfo> pIWICPixelinfo;
			pIWICmntinfo.As(&pIWICPixelinfo);
			UINT nBPP ;
			pIWICPixelinfo->GetBitsPerPixel(&nBPP);//获取图片像素的位大小的BPP（Bits Per Pixel）信息
			bmpRowByte=(((UINT)((uint64_t)bmpWidth* (uint64_t)nBPP)) + 7) / 8;// 计算图片实际的行大小（单位：字节）
			D3D12_HEAP_DESC stTextureHeapDesc = {};//创建纹理的默认堆
			//为堆指定纹理图片至少2倍大小的空间，这里没有详细去计算了，只是指定了一个足够大的空间，够放纹理就行
			//实际应用中也是要综合考虑分配堆的大小，以便可以重用堆
			stTextureHeapDesc.SizeInBytes = ((UINT)(((2 * bmpRowByte*bmpHeight) + ((D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)-1)) & ~(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1)));
			stTextureHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;//指定堆的对齐方式，这里使用了默认的64K边界对齐，因为我们暂时不需要MSAA支持
			stTextureHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;		//默认堆类型
			stTextureHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			stTextureHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			stTextureHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;//拒绝渲染目标纹理、拒绝深度蜡板纹理，实际就只是用来摆放普通纹理
			d3dDevice->CreateHeap(&stTextureHeapDesc, IID_PPV_ARGS(&pITextureHeap));
			D3D12_RESOURCE_DESC					stTextureDesc = {};//创建2D纹理
			stTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			stTextureDesc.MipLevels = 1;
			stTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			stTextureDesc.Width = bmpWidth;
			stTextureDesc.Height = bmpHeight;
			stTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			stTextureDesc.DepthOrArraySize = 1;
			stTextureDesc.SampleDesc.Count = 1;
			stTextureDesc.SampleDesc.Quality = 0;
			//使用“定位方式”来创建纹理，注意下面这个调用内部实际已经没有存储分配和释放的实际操作了，所以性能很高
			//同时可以在这个堆上反复调用CreatePlacedResource来创建不同的纹理，当然前提是它们不在被使用的时候，才考虑重用堆
			d3dDevice->CreatePlacedResource(
				pITextureHeap.Get()
				, 0
				, &stTextureDesc
				, D3D12_RESOURCE_STATE_COPY_DEST
				, nullptr
				, IID_PPV_ARGS(&pITexture)
			);
			D3D12_RESOURCE_DESC stCopyDstDesc = pITexture->GetDesc();//获取上传堆资源缓冲的大小，这个尺寸通常大于实际图片的尺寸
			
			d3dDevice->GetCopyableFootprints(&stCopyDstDesc, 0, 1, 0, nullptr, nullptr, nullptr, &n64UploadBufferSize);
			D3D12_HEAP_DESC stUploadHeapDesc = {  };//创建上传堆
			stUploadHeapDesc.SizeInBytes = ((UINT)(((2 * n64UploadBufferSize) + ((D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)-1)) & ~(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1)));//尺寸依然是实际纹理数据大小的2倍并64K边界对齐大小
			stUploadHeapDesc.Alignment = 0;//注意上传堆肯定是Buffer类型，可以不指定对齐方式，其默认是64k边界对齐
			stUploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;		//上传堆类型
			stUploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			stUploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			stUploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;//上传堆就是缓冲，可以摆放任意数据
			d3dDevice->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&pIUploadHeap));
			D3D12_RESOURCE_DESC stUploadResDesc = {};//使用“定位方式”创建用于上传纹理数据的缓冲资源
			stUploadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stUploadResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stUploadResDesc.Width = n64UploadBufferSize;
			stUploadResDesc.Height = 1;
			stUploadResDesc.DepthOrArraySize = 1;
			stUploadResDesc.MipLevels = 1;
			stUploadResDesc.Format = DXGI_FORMAT_UNKNOWN;
			stUploadResDesc.SampleDesc.Count = 1;
			stUploadResDesc.SampleDesc.Quality = 0;
			stUploadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			stUploadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			d3dDevice->CreatePlacedResource(
				pIUploadHeap.Get()
				, 0
				, &stUploadResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pITextureUpload)
			);
			//加载图片数据至上传堆，即完成第一个Copy动作，从memcpy函数可知这是由CPU完成的 按照资源缓冲大小来分配实际图片数据存储的内存大小
			void* pbPicData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, n64UploadBufferSize);
			bmpSource->CopyPixels(nullptr
				, bmpRowByte
				, static_cast<UINT>(bmpRowByte * bmpHeight)   //注意这里才是图片数据真实的大小，这个值通常小于缓冲的大小
				, reinterpret_cast<BYTE*>(pbPicData)
			);
			UINT   nNumSubresources = 1u;  //我们只有一副图片，即子资源个数为1
			UINT   nTextureRowNum = 0u;
			UINT64 n64TextureRowSizes = 0u;
			UINT64 n64RequiredSize = 0u;
			D3D12_RESOURCE_DESC					stDestDesc = pITexture->GetDesc();
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT	stTxtLayouts = {};
			d3dDevice->GetCopyableFootprints(
				&stDestDesc
				, 0
				, nNumSubresources
				, 0
				, &stTxtLayouts
				, &nTextureRowNum
				, &n64TextureRowSizes
				, &n64RequiredSize
			);
			BYTE* pData = nullptr;
			pITextureUpload->Map(0, NULL, reinterpret_cast<void**>(&pData));

			BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + stTxtLayouts.Offset;
			const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pbPicData);
			for (UINT y = 0; y < nTextureRowNum; ++y)
			{
				memcpy(pDestSlice + static_cast<SIZE_T>(stTxtLayouts.Footprint.RowPitch) * y
					, pSrcSlice + static_cast<SIZE_T>(bmpRowByte) * y
					, bmpRowByte);
			}
			pITextureUpload->Unmap(0, NULL);
			//释放图片数据，做一个干净的程序员
			::HeapFree(::GetProcessHeap(), 0, pbPicData);
			D3D12_TEXTURE_COPY_LOCATION stDstCopyLocation = {};
			stDstCopyLocation.pResource = pITexture.Get();
			stDstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			stDstCopyLocation.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION stSrcCopyLocation = {};
			stSrcCopyLocation.pResource = pITextureUpload.Get();
			stSrcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			stSrcCopyLocation.PlacedFootprint = stTxtLayouts;
			graphicsCommandList->CopyTextureRegion(&stDstCopyLocation, 0, 0, 0, &stSrcCopyLocation, nullptr);
			//设置一个资源屏障，同步并确认复制操作完成
			//直接使用结构体然后调用的形式
			D3D12_RESOURCE_BARRIER stResBar = {};
			stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			stResBar.Transition.pResource = pITexture.Get();
			stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			graphicsCommandList->ResourceBarrier(1, &stResBar);
			// 执行命令列表并等待纹理资源上传完成，这一步是必须的
			graphicsCommandList->Close();
			ID3D12CommandList* ppCommandLists[] = { graphicsCommandList.Get() };
			commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
			return;
		}
	}
	throw L"图片格式不支持";
}

void D3DApp::draw()
{
	Vertex vertex[] =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f,0.0f,1.0f,1.0f)}),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(0.0f,0.0f,1.0f,1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(0.0f,0.0f,1.0f,1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f,0.0f,1.0f,1.0f) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(0.0f,0.0f,1.0f,1.0f) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(0.0f,0.0f,1.0f,1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(0.0f,0.0f,1.0f,1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(0.0f,0.0f,1.0f,1.0f) })
	};
	USHORT indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};
	D3DCreateBlob(sizeof(Vertex) * _countof(vertex), &cube.vertexBufferCPU);
	RtlCopyMemory(cube.vertexBufferCPU->GetBufferPointer(), vertex, sizeof(Vertex) * _countof(vertex));
	D3DCreateBlob(sizeof(USHORT) * _countof(indices), &cube.indexBufferCPU);
	RtlCopyMemory(cube.indexBufferCPU->GetBufferPointer(), indices, sizeof(USHORT) * _countof(indices));
	cube.vertexBufferGPU = createGeometryResource(_countof(vertex) * sizeof(Vertex), vertex, cube.vertexUploadBuffer);
	cube.indexBufferGPU = createGeometryResource(_countof(indices) * sizeof(USHORT), indices, cube.indexUploadBuffer);
	cube.vertexBufferView.BufferLocation = cube.vertexBufferGPU->GetGPUVirtualAddress();
	cube.vertexBufferView.StrideInBytes = sizeof(Vertex);
	cube.vertexBufferView.SizeInBytes = sizeof(Vertex) * _countof(vertex);
	cube.indexBufferView.BufferLocation = cube.indexBufferGPU->GetGPUVirtualAddress();
	cube.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	cube.indexBufferView.SizeInBytes = sizeof(USHORT) * _countof(indices);
	cube.indexCount = _countof(indices);
}

void D3DApp::onReSize(UINT newWindowWidth,UINT newWindowHeight) {
	windowWidth = newWindowWidth;
	windowHeight = newWindowHeight;
	flushCommandQueue();
	/*
		在重置交换链缓冲区大小的同时必须进行CPU与GPU之间的同步
		也就是说，不能同时在一个时间点上既调用render()渲染函数又调用onResize()
		这是一个资源同步的问题，在渲染函数中，交换链缓冲区还在交替缓冲区RTV资源
		而onResize()这边就重置缓冲区大小了，就会引发错误
	*/
	graphicsCommandList->Reset(commandAllocator.Get(), nullptr);
	d3d11DeviceContext->Flush();//擦除上一帧的D3D11上下文
	d2dDeviceContext->SetTarget(nullptr);
	for (auto& i : d2dBitmap) 
		i.Reset();
	for (auto& i : d3d11WrappedResource) 
		i.Reset();
	for (auto& i : swapChainBufferResource) 
		i.Reset();
	dsvResource.Reset();
	HRESULT s=swapChain->ResizeBuffers(
		swapChainBufferCount
		, windowWidth
		, windowHeight
		,DXGI_FORMAT_R8G8B8A8_UNORM
		,DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	);
	if (s != S_OK) {
		throw L"交换链重置失败了，原因是因为d3d11WrappedResource的某处资源仍然引用着swapChainBufferResource，必须清理干净才能重置交换链缓冲区大小";
	}
	currentSwapChainBufferNO = 0;
	D3D11_RESOURCE_FLAGS stD3D11Flags = {  };
	stD3D11Flags.BindFlags = D3D11_BIND_RENDER_TARGET;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapCPUHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < swapChainBufferCount; i++) {//遍历交换链缓冲区，在每个交换链缓冲区上创建RTV
		swapChain->GetBuffer(//根据索引获取交换链缓冲区资源
			i,//索引
			IID_PPV_ARGS(&swapChainBufferResource[i])//得到该缓冲区上的资源
		);
		d3dDevice->CreateRenderTargetView(//在当前缓冲区上创建RTV
			swapChainBufferResource[i].Get(),
			nullptr,
			rtvHeapCPUHandle
		);
		d3d11On12Device->CreateWrappedResource(//此方法创建用于 D3D 11on12 的 D3D11 资源。
			swapChainBufferResource[i].Get()
			/*
				将D3D12的RTV资源转换成D3D11资源，但最终交换链提交呈现的还是D3D12的RTV
				因此这个方法应当是将RTV绑定到D3D11资源对象，我们需要由D3D11完成D2D绘制
			*/
			, &stD3D11Flags
			, D3D12_RESOURCE_STATE_RENDER_TARGET
			, D3D12_RESOURCE_STATE_PRESENT
			, IID_PPV_ARGS(&d3d11WrappedResource[i])
		);
		ComPtr<IDXGISurface> pIDXGISurface;
		d3d11WrappedResource[i].As(&pIDXGISurface);
		d2dDeviceContext->CreateBitmapFromDxgiSurface(
			pIDXGISurface.Get()
			, &BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW
				/*
							D2D1_BITMAP_OPTIONS_NONE = 0x00000000,位图是使用默认属性创建的。
						  D2D1_BITMAP_OPTIONS_TARGET = 0x00000001,位图可用作设备上下文目标。
						  D2D1_BITMAP_OPTIONS_CANNOT_DRAW = 0x00000002,位图不能用作输入。
						  D2D1_BITMAP_OPTIONS_CPU_READ = 0x00000004,可以从 CPU 读取位图。
						  D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE = 0x00000008,位图与ID2D1GdiInteropRenderTarget::GetDC一起使用。
						  D2D1_BITMAP_OPTIONS_FORCE_DWORD = 0xffffffff
				*/
				, PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)//位图的像素格式和 alpha 模式。
				, 96.0f//位图的水平 dpi。默认值为 96.0f。
				, 96.0f//位图的垂直 dpi。默认值为 96.0f。
				//, _In_opt_ ID2D1ColorContext *colorContext = NULL//用于与位图一起使用的颜色上下文。如果您不想指定颜色上下文，请将此参数设置为NULL。默认为NULL
			)
			, &d2dBitmap[i]
		);
		rtvHeapCPUHandle.ptr += rtvHeapSize;//移动指针到下一个缓冲区
	}
	D3D12_CLEAR_VALUE optClear={};
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	for(int i=0;i<_countof(optClear.Color);i++)
		optClear.Color[i] = rtvBackgroundColorHSLA[i];
	optClear.DepthStencil.Depth = 1.0f;//指定深度值。
	optClear.DepthStencil.Stencil = 0;//指定模板值。
	d3dDevice->CreateCommittedResource(
		&createD3D12_HEAP_PROPERTIES(),
		D3D12_HEAP_FLAG_NONE,
		&createD3D12_RESOURCE_DESC(windowWidth, windowHeight, D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0
			, 1, 1, DXGI_FORMAT_R24G8_TYPELESS, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN
			, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(dsvResource.GetAddressOf())
	);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc={};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	d3dDevice->CreateDepthStencilView(
		dsvResource.Get()
		, &dsvDesc
		, dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);
	graphicsCommandList->ResourceBarrier(1, &createD3D12_RESOURCE_BARRIER(dsvResource.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	graphicsCommandList->Close();
	ID3D12CommandList* cmdsLists[] = { graphicsCommandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	flushCommandQueue();
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = static_cast<float>(windowWidth);
	viewPort.Height = static_cast<float>(windowHeight);
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	rect = { 0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
	XMStoreFloat4x4(
		&mProj
		, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(windowWidth) / windowHeight, 1.0f, 1000.0f)
	);
}

void D3DApp::flushCommandQueue()
{
	currentFenceValue++;
	HRESULT f=commandQueue->Signal(fence.Get(), currentFenceValue);
	if (fence->GetCompletedValue() < currentFenceValue){
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		f=fence->SetEventOnCompletion(currentFenceValue, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ComPtr<ID3D12Resource> D3DApp::createGeometryResource(UINT64 RowPitch,const void* pData, ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr < ID3D12Resource> defaultBuffer;
	d3dDevice->CreateCommittedResource(
		&createD3D12_HEAP_PROPERTIES(),
		D3D12_HEAP_FLAG_NONE,
		&createD3D12_RESOURCE_DESC(RowPitch,1, D3D12_RESOURCE_DIMENSION_BUFFER, 0, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR),
		D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_DEPTH_WRITE D3D12_RESOURCE_STATE_COMMON
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())
	);
	d3dDevice->CreateCommittedResource(
		&createD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&createD3D12_RESOURCE_DESC(RowPitch,1, D3D12_RESOURCE_DIMENSION_BUFFER,0,1,1, DXGI_FORMAT_UNKNOWN,1,0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR),
		D3D12_RESOURCE_STATE_GENERIC_READ,//GPU会从这个缓冲区中读取，并将其内容复制到默认的堆中
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())
	);
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = pData;
	subResourceData.RowPitch = RowPitch;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	graphicsCommandList->ResourceBarrier(1, &createD3D12_RESOURCE_BARRIER(defaultBuffer.Get(), 
	D3D12_RESOURCE_STATE_COMMON , D3D12_RESOURCE_STATE_COPY_DEST));
	ID3D12Device* pDevice; 
	defaultBuffer->GetDevice(IID_PPV_ARGS(&pDevice));
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT pLayouts;
	UINT pNumRows;
	UINT64 pRowSizeInBytes, pTotalBytes;
	pDevice->GetCopyableFootprints(&defaultBuffer->GetDesc(),
		0, 1, 0, &pLayouts, &pNumRows, &pRowSizeInBytes, &pTotalBytes);
	pDevice->Release();
	BYTE* tt;
	uploadBuffer->Map(0, NULL, reinterpret_cast<void**>(&tt));
	D3D12_MEMCPY_DEST pDest = { tt + pLayouts.Offset, pLayouts.Footprint.RowPitch,
		pLayouts.Footprint.RowPitch * pNumRows };
	RtlCopyMemory(
		reinterpret_cast<BYTE*>(pDest.pData),
		reinterpret_cast<const BYTE*>(subResourceData.pData), pRowSizeInBytes
	);
	uploadBuffer->Unmap(0, NULL);
	graphicsCommandList->CopyBufferRegion(
		defaultBuffer.Get(), 0, uploadBuffer.Get(), pLayouts.Offset, pLayouts.Footprint.Width);
	graphicsCommandList->ResourceBarrier(1, &createD3D12_RESOURCE_BARRIER(defaultBuffer.Get(),
	D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	return defaultBuffer;
}

D3D12_DESCRIPTOR_HEAP_DESC D3DApp::createD3D12_DESCRIPTOR_HEAP_DESC(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags, UINT NodeMask)
{
	D3D12_DESCRIPTOR_HEAP_DESC d={};
	d.Type = Type;
	d.NumDescriptors = NumDescriptors;
	d.Flags = Flags;
	d.NodeMask = NodeMask;
	return d;
}

D3D12_HEAP_PROPERTIES D3DApp::createD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE Type, D3D12_CPU_PAGE_PROPERTY CPUPageProperty, D3D12_MEMORY_POOL MemoryPoolPreference, UINT CreationNodeMask, UINT VisibleNodeMask)
{
	D3D12_HEAP_PROPERTIES stHeapProp = {  };
	stHeapProp.Type = Type;
	stHeapProp.CPUPageProperty = CPUPageProperty;
	stHeapProp.MemoryPoolPreference = MemoryPoolPreference;
	stHeapProp.CreationNodeMask = CreationNodeMask;
	stHeapProp.VisibleNodeMask = VisibleNodeMask;
	return stHeapProp;
}

D3D12_RESOURCE_DESC D3DApp::createD3D12_RESOURCE_DESC(UINT64 Width, UINT Height, D3D12_RESOURCE_DIMENSION Dimension, UINT64 Alignment,   UINT16 DepthOrArraySize, UINT16 MipLevels, DXGI_FORMAT Format, UINT Count, UINT Quality, D3D12_TEXTURE_LAYOUT Layout, D3D12_RESOURCE_FLAGS Flags)
{
	D3D12_RESOURCE_DESC stResSesc = {};
	stResSesc.Dimension = Dimension;
	stResSesc.Alignment = Alignment;
	stResSesc.Width = Width;
	stResSesc.Height = Height;
	stResSesc.DepthOrArraySize = DepthOrArraySize;
	stResSesc.MipLevels = MipLevels;
	stResSesc.Format = Format;
	stResSesc.SampleDesc.Count = Count;
	stResSesc.SampleDesc.Quality = Quality;
	stResSesc.Layout = Layout;
	stResSesc.Flags = Flags;
	return stResSesc;
}

D3D12_RESOURCE_BARRIER D3DApp::createD3D12_RESOURCE_BARRIER(ID3D12Resource* pResource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter, D3D12_RESOURCE_BARRIER_TYPE Type, D3D12_RESOURCE_BARRIER_FLAGS Flags, UINT Subresource)
{
	D3D12_RESOURCE_BARRIER d={};
	RtlZeroMemory(&d, sizeof(d));
	d.Transition.pResource = pResource;
	d.Transition.Subresource = Subresource;
	d.Transition.StateBefore = StateBefore;
	d.Transition.StateAfter = StateAfter;
	d.Type = Type;
	d.Flags = Flags;
	return d;
}

void D3DApp::compileSharder(WCHAR* shaderFilePath)
{
	D3DCompileFromFile(//编译着色器
		shaderFilePath,//着色器文件路径
		nullptr, //定义着色器宏的D3D_SHADER_MACRO结构的可选数组。每个宏定义都包含一个名称和一个以 null 结尾的定义。如果不使用，设置为NULL。数组中的最后一个结构用作终止符，并且必须将所有成员设置为NULL。
		D3D_COMPILE_STANDARD_FILE_INCLUDE, //指向编译器用来处理包含文件的ID3DInclude接口的可选指针。如果将此参数设置为NULL并且着色器包含 #include，则会发生编译错误。您可以传递D3D_COMPILE_STANDARD_FILE_INCLUDE宏，它是一个指向默认包含处理程序的指针。此默认包含处理程序包括与当前目录相关的文件。
		"VSMain", //着色器入口点函数
		"vs_5_0", //着色器目标或着色器功能集 5为版本
		D3DCOMPILE_DEBUG| D3DCOMPILE_SKIP_OPTIMIZATION //启用着色器编译调试
		//,|D3DCOMPILE_PACK_MATRIX_ROW_MAJOR,//编译为行矩阵形式
		,0
		,&vsData//编译成功，得到该着色器的数据
		,nullptr//着色器编译错误时，将错误信息给到此输出参数
	);
	D3DCompileFromFile(shaderFilePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION/* | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR*/, 0, &psData, nullptr);
}

D3DApp::D3DApp(HWND hWnd, UINT windowWidth, UINT windowHeight,  WCHAR* shaderFilePath ,BOOL Windowed, BOOL debug)
	:hWnd(hWnd),windowWidth(windowWidth), windowHeight(windowHeight)
{
	if (debug) {
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		debugController->EnableDebugLayer();//启动调试器
	}
	initd3dDevice();//创建设备
	initfence();
	initcommandQueue();//创建命令队列
	initcommandAllocator();//创建命令分配器
	initgraphicsCommandList();//创建图形命令列表
	initdxgiSwapChain(Windowed);//创建交换链
	initrtvHeap();//创建RTV描述符堆
	initdsvHeap();//创建DSV描述符堆
	rtvHeapSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	initD2D();
	onReSize(windowWidth,windowHeight);
	graphicsCommandList->Reset(commandAllocator.Get(), nullptr);
	initcbvHeap();//创建CBV描述符堆
	//initsrvHeap();
	initrootSignature();//创建根签名
	compileSharder(shaderFilePath);//编译着色器
	draw();
	initpipelineState();//创建渲染管线状态
	graphicsCommandList->Close();
	ID3D12CommandList* cmdsLists[] = { graphicsCommandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	flushCommandQueue();
	CoInitialize(//在当前线程上初始化 COM 库并将并发模型标识为单线程单元 (STA)。
		nullptr//保留参数，只能为null
	);
	CoCreateInstance(//创建WIC工厂
		CLSID_WICImagingFactory, //与将用于创建对象的数据和代码关联的 CLSID。
		nullptr, //如果为NULL，则表示该对象不是作为聚合的一部分创建的。如果非NULL，则指向聚合对象的IUnknown接口
		CLSCTX_INPROC_SERVER, //管理新创建对象的代码将运行的上下文。这些值取自枚举 CLSCTX
		IID_PPV_ARGS(&wicFactory)
	);
}

D3DApp::~D3DApp(){
	//释放资源
	pipelineState.Reset();
	rootSignature.Reset();
	cbvHeap.Reset();
	dsvHeap.Reset();
	for (auto& i : swapChainBufferResource)
		i.Reset();
	rtvHeap.Reset();
	swapChain.Reset();
	graphicsCommandList.Reset();
	commandAllocator.Reset();
	commandQueue.Reset();
	d3dDevice.Reset();
	debugController.Reset();
}

void D3DApp::render()
{
	////////////////////////////////////////////////////////////////////////////更新CBV相机
	XMMATRIX view = XMMatrixLookAtLH(
		cameraPosition//相机自身的position
		, lookAtPoint//相机lookAt的position
		, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)//相机的向上方向，通常设为< 0.0f, 1.0f, 0.0f >。
	);
	XMStoreFloat4x4(//把M给pDestination
		&mView//out pDestination	左边float4*4
		, view//in M	右边矩阵
	);
	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;
	MVP objConstants;
	XMStoreFloat4x4(&objConstants.mvp, XMMatrixTranspose(worldViewProj));
	RtlCopyMemory(&mMappedData[0], &objConstants,sizeof(MVP));
	////////////////////////////////////////////////////////////////////////////图形渲染
	commandAllocator->Reset();//开始记录接下来的一系列命令，并清除之前记录且没有被执行的命令
	/*
	* 向GPU提交了一整帧的渲染命令后，我们可能还要为了绘制下一帧而复用命令分配器中的内存
	ID3D12CommandAllocator::Reset方法由此应运而生。然而由于命令队列可能会引用命令分配器中的数据
	所以在没有确定GPU执行完命令分配器中的所有命令之前，千万不要重置命令分配器！
	*/
	graphicsCommandList->Reset(commandAllocator.Get(), pipelineState.Get());//注意调用的是ID3D12GraphicsCommandList::Reset()而不是ComPtr::Reset()
	/*
	* 此方法将命令列表恢复为刚创建时的初始状态，我们可以借此继续复用其低层内存
	也可以避免释放旧列表再创建新列表这一系列的烦琐操作。注意，重置命令列表并不会影响命令队列中的命令
	因为相关的命令分配器仍在维护着其内存中被命令队列引用的系列命令。
	*/
	graphicsCommandList->RSSetViewports(//设置视图端口
		1, 
		&viewPort
	);
	graphicsCommandList->RSSetScissorRects(//设置矩形视图
		1, 
		&rect
	);
	graphicsCommandList->ResourceBarrier(
		1
		,&createD3D12_RESOURCE_BARRIER(swapChainBufferResource[currentSwapChainBufferNO].Get()
			, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)
	);
	/*
		这段代码将表示我们在屏幕上显示的图像从D3D12_RESOURCE_STATE_PRESENT状态转换为D3D12_RESOURCE_STATE_RENDER_TARGET状态
		注意，资源屏障被添加到命令列表中。你可以把资源屏障转换看作是命令本身，告诉GPU资源的状态正在转换
		这样它就可以在执行后续命令时采取必要的步骤来防止资源危险
	*/
	//graphicsCommandList->SetPipelineState(pipelineState.Get());//设置渲染管线
	graphicsCommandList->ClearRenderTargetView(//擦除指定交换链缓冲区上的RTV
		getD3D12_CPU_DESCRIPTOR_HANDLE(),//从哪个RTV上开始擦起？这个方法会从当前指定的CPUHANDLE开始擦除至末尾的RTV
		rtvBackgroundColorHSLA,//擦除时使用的颜色
		0,//pRects参数指定的数组中矩形的数量。
		nullptr//要擦除的矩形区域，null表示擦除当前整个RTV，给定一个D3D12_RECT*擦除部分矩形区域
	);
	graphicsCommandList->ClearDepthStencilView(//擦除DSV
		dsvHeap->GetCPUDescriptorHandleForHeapStart()//描述 CPU 描述符句柄，它表示要清除的深度模板的堆的开始。
		, D3D12_CLEAR_FLAG_DEPTH //表示应清除深度缓冲区。
		| D3D12_CLEAR_FLAG_STENCIL //表示应清除模板缓冲区。
		, 1.0f//用于清除深度缓冲区的值。
		, 0//用于清除模板缓冲区的值。
		, 0//pRects参数指定的数组中矩形的数量。
		, nullptr//用于清除资源视图中矩形的D3D12_RECT结构数组。如果为NULL，ClearDepthStencilView清除整个DSV
	);
	graphicsCommandList->OMSetRenderTargets(//设置渲染目标
		1,
		&getD3D12_CPU_DESCRIPTOR_HANDLE(),
		TRUE,
		&(dsvHeap->GetCPUDescriptorHandleForHeapStart())
	);
	//graphicsCommandList->OMSetBlendFactor(rtvBackgroundColorHSLA);
	ID3D12DescriptorHeap* ppHeaps[] = { cbvHeap.Get() };
	graphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	graphicsCommandList->SetGraphicsRootSignature(rootSignature.Get());//设置根签名
	graphicsCommandList->IASetVertexBuffers(//设置顶点缓冲
		0, 
		1,
		&cube.vertexBufferView
	);
	graphicsCommandList->IASetIndexBuffer(&cube.indexBufferView);//设置索引缓冲
	/*
		索引缓冲的概念就是把每个顶点按照序号排序作为顶点的索引，然后依次将这些顶点连接起来组成网格几何体
		绘制连接的方式使用三角形连接，也就是每3个顶点连接
		除此之外，绘制顺序也跟图形命令列表的执行命令顺序有关
		最后一个绘制的命令覆盖之前的绘制命令，也就是先绘制的在下面，后绘制的在上面，就近原则
	*/
	graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	graphicsCommandList->SetGraphicsRootDescriptorTable(
		0
		, cbvHeap->GetGPUDescriptorHandleForHeapStart()
	);
	graphicsCommandList->DrawIndexedInstanced(//按照索引缓冲绘制连接
		cube.indexCount,//从每个实例的索引缓冲区读取的索引数。
		1,//要绘制的实例数。
		0,//GPU 从索引缓冲区读取的第一个索引的位置。
		0,//在从顶点缓冲区读取顶点之前添加到每个索引的值。
		0//在从顶点缓冲区读取每个实例数据之前添加到每个索引的值。
	);
	/*
	 切换渲染目标纹理的状态，从渲染状态（In 即写入状态）变成呈现状态（Out 即读出状态）
	 因为后面我们用了D2D和DWrite来输出2D对象或文本，所以不用切换了，D2D关联的D3D11on12设备内部会帮我们做这个操作
	graphicsCommandList->ResourceBarrier(
		1
		,&createD3D12_RESOURCE_BARRIER(swapChainBufferResource[currentSwapChainBufferNO].Get()
			,D3D12_RESOURCE_STATE_RENDER_TARGET , D3D12_RESOURCE_STATE_PRESENT)
	);
	*/
	graphicsCommandList->Close();//关闭图形命令列表，可以去执行了
	/*
	* 虽然这些方法的名字看起来像是会使对应的命令立即执行，但事实却并非如此，
	上面的代码仅仅是将命令加入命令列表而已。调用 ExecuteCommandLists 方法才会将命令真正地送入命令队列
	供GPU在合适的时机处理。当命令都被加入命令列表之后，如上述的最终代码
	我们必须调用ID3D12GraphicsCommandList::Close 方法来结束命令的记录
	在调用 ID3D12CommandQueue::ExecuteCommandLists 方法提交命令列表之前，一定要将其关闭。
	*/
	ID3D12CommandList* ppCommandLists[] = { graphicsCommandList.Get() };//ID3D12CommandList是ID3D12GraphicsCommandList的基类
	commandQueue->ExecuteCommandLists(//执行命令列表
		_countof(ppCommandLists) // ppCommandLists命令列表数组中命令列表的数量
		, ppCommandLists//这个参数的类型要求是ID3D12CommandList *const *，其不能转换为ID3D12GraphicsCommandList**，所以使用它的基类
	);
	//flushCommandQueue();
	static WCHAR str2d[] = L"";
	StringCchPrintfW(//实时渲染更新待绘制的D2D文字
		str2d
		, _countof(D2DString)
		, D2DString
		, GPUcardName
		, XMVectorGetByIndex(cameraPosition, 0), XMVectorGetByIndex(cameraPosition, 1), XMVectorGetByIndex(cameraPosition, 2), XMVectorGetByIndex(cameraPosition, 3)
		, XMVectorGetByIndex(lookAtPoint, 0), XMVectorGetByIndex(lookAtPoint, 1), XMVectorGetByIndex(lookAtPoint, 2), XMVectorGetByIndex(lookAtPoint, 3)
		, get_cameraPosition_lookAtPoint_between_StraightDistance()
		, objConstants.mvp._11, objConstants.mvp._12, objConstants.mvp._13, objConstants.mvp._14
		, objConstants.mvp._21, objConstants.mvp._22, objConstants.mvp._23, objConstants.mvp._24
		, objConstants.mvp._31, objConstants.mvp._32, objConstants.mvp._33, objConstants.mvp._34
		, objConstants.mvp._41, objConstants.mvp._42, objConstants.mvp._43, objConstants.mvp._44
		, mWorld._11, mWorld._12, mWorld._13, mWorld._14
		, mWorld._21, mWorld._22, mWorld._23, mWorld._24
		, mWorld._31, mWorld._32, mWorld._33, mWorld._34
		, mWorld._41, mWorld._42, mWorld._43, mWorld._44
		, mView._11, mView._12, mView._13, mView._14
		, mView._21, mView._22, mView._23, mView._24
		, mView._31, mView._32, mView._33, mView._34
		, mView._41, mView._42, mView._43, mView._44
		, mProj._11, mProj._12, mProj._13, mProj._14
		, mProj._21, mProj._22, mProj._23, mProj._24
		, mProj._31, mProj._32, mProj._33, mProj._34
		, mProj._41, mProj._42, mProj._43, mProj._44
	);
	d3d11On12Device->AcquireWrappedResources(//获取用于 D3D 11on12 的 D3D11 资源。指示可以重新开始对包装资源的呈现。
		d3d11WrappedResource[currentSwapChainBufferNO].GetAddressOf()
		, 1
	);
	d2dDeviceContext->SetTarget(d2dBitmap[currentSwapChainBufferNO].Get());
	d2dDeviceContext->BeginDraw();//开始绘制
	d2dDeviceContext->SetTransform(Matrix3x2F::Identity());
	D2D1_RECT_F d2dRect = RectF(
		0.0f//与左边的间隔距离（相当于HTML中的margin-left）
		, 150.0f//与上面的间隔距离（相当于HTML中的margin-top）
		, static_cast<float>(windowWidth)//行宽（相当于HTML中的width）
		, 500.0f//行高（相当于HTML中的line-height）
	);
	d2dDeviceContext->DrawTextW(
		str2d
		,wcslen(str2d)
		,writeTextFormat.Get()
		,&d2dRect
		,d2dSolidColorBrush.Get()
	);
	float clipx = mView._11*0 + mView._12*0 + mView._13*0 + objConstants.mvp._14;
	float clipy = mView._11 * 0 + mView._12 * 0 + mView._13 * 0 + objConstants.mvp._24;
	float clipz = mView._11 * 0 + mView._12 * 0 + mView._13 * 0 + objConstants.mvp._34;
	float clipw = mView._11 * 0 + mView._12 * 0 + mView._13 * 0 + objConstants.mvp._44;
	if (clipw < 0) {
		return;
	}
	//3.计算NDC坐标：（透视分割算法：用剪切坐标XYZ除以W即可）

	float	NDC坐标x = clipx / clipw;
	float	NDC坐标y = clipy / clipw;
	float	NDC坐标z = clipz / clipw;

	//	4.NDC坐标转屏幕坐标：
	D2D1_POINT_2F dp;
	dp.x= static_cast<float>(windowWidth) / 2 * NDC坐标x + NDC坐标x + static_cast<float>(windowWidth) / 2;
	dp.y = -(static_cast<float>(windowHeight) / 2 * NDC坐标y) + NDC坐标y + static_cast<float>(windowHeight) / 2;


	d2dDeviceContext->DrawLine(
		{
			static_cast<float>(windowWidth/2)
			,static_cast<float>(windowHeight)
		}
		,dp
		,d2dSolidColorBrush.Get()
	);
	d2dDeviceContext->EndDraw();//结束绘制
	d3d11On12Device->ReleaseWrappedResources(d3d11WrappedResource[currentSwapChainBufferNO].GetAddressOf(), 1);
	d3d11DeviceContext->Flush();//擦除上一帧的D3D11上下文
	swapChain->Present(0, 0);//最后提交画面，这一帧渲染完毕
	currentSwapChainBufferNO = ((currentSwapChainBufferNO + 1) % swapChainBufferCount);//缓冲区交替渲染
	flushCommandQueue();
}

void D3DApp::OnLeftMouseDown(int x, int y)
{
	isLeftDown = true;
	lastRecordMousePosition.x = x;
	lastRecordMousePosition.y = y;
	SetCapture(
		hWnd// 当前线程中要捕获鼠标的窗口句柄。
	);
	/*
		此函数将鼠标或样式捕获设置为属于当前线程的指定窗口。为窗口调用此函数后
		窗口将捕获鼠标光标停留在窗口内时发生的鼠标输入
		该窗口还捕获当鼠标光标位于窗口内时用户按下鼠标按钮并在移动鼠标时继续按住鼠标按钮时发生的鼠标输入
		一次只有一个窗口可以捕获鼠标或触控笔。
	*/
}

void D3DApp::OnLeftMouseUp(int x, int y)
{
	isLeftDown = false;
	ReleaseCapture();//此函数从当前线程中的窗口释放鼠标或触笔捕获并恢复输入的正常处理。
}



void D3DApp::OnRightMouseDown(int x, int y)
{
	isRightDown = true;
	lastRecordMousePosition.x = x;
	lastRecordMousePosition.y = y;
	SetCapture(
		hWnd// 当前线程中要捕获鼠标的窗口句柄。
	);
}

void D3DApp::OnRightMouseUp(int x, int y)
{
	isRightDown = false;
	ReleaseCapture();//此函数从当前线程中的窗口释放鼠标或触笔捕获并恢复输入的正常处理。
}



void D3DApp::OnMouseMove(int x, int y)
{
	/*
		MapControllerCamera有以下3种控制
		1.鼠标左键移动时同时平移lookAtPoint和cameraPosition
		2.鼠标右键移动时使cameraPosition沿着lookAtPoint左右旋转、上下移动
		3.鼠标滚轮缩放cameraPosition的Z
	*/	
	float step = 0.25;
	float dx = XMConvertToRadians(step * static_cast<float>(x - lastRecordMousePosition.x));
	float dy = XMConvertToRadians(step * static_cast<float>(y - lastRecordMousePosition.y));
	if (isLeftDown) {
		lookAtPoint=XMVectorSet(
			XMVectorGetByIndex(lookAtPoint, 0)- dx
			, XMVectorGetByIndex(lookAtPoint, 1)+dy
			, XMVectorGetByIndex(lookAtPoint, 2)
			, XMVectorGetByIndex(lookAtPoint, 3)
		);
		cameraPosition = XMVectorSet(
			XMVectorGetByIndex(cameraPosition, 0) - dx
			, XMVectorGetByIndex(cameraPosition, 1) + dy
			, XMVectorGetByIndex(cameraPosition, 2)
			, XMVectorGetByIndex(cameraPosition, 3)
		);
	}
	else if (isRightDown) {
		/*
			0		10		20		30		45		60			80		90		100
			0		0.17	0.34	0.5		0.7		0.86		0.98	1		0.98
		*/
		mTheta += dx;
		//mPhi += dy;
		/*
			dx		mPhi	mTheta
			0.17	0.78	4.71
					0.01	0.99
		*/
		float xx = mRadius 
			//* sinf(mPhi) 
			* cosf(mTheta);
		float zz = mRadius 
			//* sinf(mPhi)
			* sinf(mTheta);
		cameraPosition = XMVectorSet(
			//XMVectorGetByIndex(cameraPosition, 0)+ xx
			 xx
			, XMVectorGetByIndex(cameraPosition, 1)+ dy*7.f
			//, XMVectorGetByIndex(cameraPosition, 2) + zz
			,zz
			, XMVectorGetByIndex(cameraPosition, 3)
		);
	}
	lastRecordMousePosition.x = x;
	lastRecordMousePosition.y = y;
}

void D3DApp::OnMouseWheel(float step)
{
	//增加焦距
	cameraPosition = XMVectorSet(
		XMVectorGetByIndex(cameraPosition, 0)
		, XMVectorGetByIndex(cameraPosition, 1)
		, XMVectorGetByIndex(cameraPosition, 2)+step
		, XMVectorGetByIndex(cameraPosition, 3)
	);
}

float D3DApp::get_cameraPosition_lookAtPoint_between_StraightDistance()
{
	//运用2次勾股定理，计算直线距离
	float xc = fabsf(
		XMVectorGetByIndex(cameraPosition, 0) - XMVectorGetByIndex(lookAtPoint, 0)
	);
	float yc = fabsf(
		XMVectorGetByIndex(cameraPosition, 1) - XMVectorGetByIndex(lookAtPoint, 1)
	);
	float zc = fabsf(
		XMVectorGetByIndex(cameraPosition, 2) - XMVectorGetByIndex(lookAtPoint, 2)
	);
	//根据勾股定理：xc的平方 + yc的平方 = 斜边的平方;那么代入公式：
	float planeStraightDistance = sqrtf(powf(xc, 2.0f) + powf(yc, 2.f));
	float stereoscopicStraightDistance = sqrtf(powf(planeStraightDistance, 2.0f) + powf(zc, 2.f));
	//XMVector3Cross(cameraPosition, lookAtPoint);叉乘
	return stereoscopicStraightDistance;
}