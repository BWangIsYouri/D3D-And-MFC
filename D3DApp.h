#include <d3d12.h>//D3D12����ͷ�ļ�
#include <dxgi.h>//dxgi��R3����Ⱦ�ӿڵĵײ�
#include <DirectXMath.h>//ʹ��D3D�ṩ����ѧ�㷨
#include <d3dcompiler.h>//������ɫ������ͷ�ļ�
#include <dxgidebug.h>//DXGI������
#include <d3d11on12.h>//D3D12��Ҫʹ��D3D11�ⴴ��D2D�����Լ�DWrite
#include <d2d1_3.h>//2D����ͼ��
#include <dwrite.h>//2D��������
#include <strsafe.h>  //for StringCchxxxxx function
#include <wincodec.h>//Windows Image Component��Windowsͼ�������ͼƬ�����
#include <wrl.h>//����WRL�⣨Windows Runtime Library���е�COM�ӿڣ�Ϊ��ʹ��COM����ָ��
/*
	D3D12��ʹ��COM�ӿڣ�Component Object Model �������ģ�ͣ���Ƶ�
* Ϊ�˸����û�����COM������������ڣ����ǿ��԰���������COM���������ָ�롣
��һ��ComPtrʵ������������Χʱ��������Զ�������ӦCOM�����Release������
* ����ָ��������->*������������ǿ��԰�����ָ�뵱��Cָ������ʹ��
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

class Vertex {//������
public:
	XMFLOAT3 position;//XMFLOAT3Ҳ����float[3]���飬3������X��Y��Z
	XMFLOAT4 color;//R��G��B��A
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
	XMFLOAT4X4 mvp = //�����Model View Projection(MVP)����.
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

	FLOAT rtvBackgroundColorHSLA[4] = {//HSLֵ��Hue ɫ��Saturation ���Ͷ�Luminance ����
		0.0f//R
		,0.0f//G
		,0.0f//B
		,0.0f//A
	};


	D3DApp(
		HWND hWnd//���ĸ������ϻ��ƣ�����˴��ڵľ��
		, UINT windowWidth//���ڿ��
		, UINT windowHeight//���ڸ߶�
		, WCHAR* shaderFilePath//��ɫ���ļ���
		, BOOL Windowed=TRUE//TRUE����ģʽ	falseȫ��ģʽ
		,BOOL debug = TRUE//TRUE���õ��� false������
	);
	virtual ~D3DApp();


	void loadtexture(LPCWSTR texcuteFilePath);
	/*
		���´��ڴ�С����Ҫ�����漰������������У����������������������µ�RTV
		�������DSV�������µ���Դ���ϵȵȣ���˱����洰�ڶ���Ӧ
	*/
	void onReSize(UINT newWindowWidth, UINT newWindowHeight);
	void OnLeftMouseDown(int x, int y);
	void OnLeftMouseUp(int x, int y);
	void OnRightMouseDown(int x, int y);
	void OnRightMouseUp(int x, int y);
	void OnMouseMove(int x, int y);
	void OnMouseWheel(
		float step//�Ƽ���������0.5		+0.5��ǰ�ƶ�		-0.5����ƶ�
	);
	void render();//��Ⱦ
private:
	ComPtr<ID3D12Debug> debugController;//DX����������
	ComPtr<IDXGIFactory1>dxgiFactory;//IDXGIFactory������DX�еĹ��������ṩ���ֺ���
	ComPtr < ID3D12Device> d3dDevice;//ID3D12Device�豸�����豸����������������
	/*
	* ÿ��GPU������ά����һ��������У�command queue���������ǻ��λ���������
	CPU�����������б�command list���������ύ�����������ȥ����һϵ������ύ���������֮ʱ��
	���ǲ����ᱻGPU����ִ�С�����GPU�������ڴ�����ǰ������������ڵ������ˣ�
	�����µ��������һֱ���������֮�еȴ�ִ�С�
	����һ���������б��йص���ΪID3D12CommandAllocator���ڴ������ӿڡ�
	��¼�������б��ڵ�����ʵ�����Ǵ洢����֮�����������������command allocator����
	��ͨ��ID3D12CommandQueue::ExecuteCommandLists����ִ�������б��ʱ��������оͻ����÷�����������
	*/
	ComPtr < ID3D12CommandQueue> commandQueue;//������ж���
	ComPtr <ID3D12GraphicsCommandList> graphicsCommandList;//ͼ�������б����
	ComPtr <ID3D12CommandAllocator> commandAllocator;//�������������
	/*
	* Ϊ�˱�����Ⱦ�г��ֿ�֡��������ý�һ֡�����ػ�����һ�ֳ�Ϊ��̨�����������������ڡ�
		����̨�������еĶ���֡�������֮�󣬺�̨��������ǰ̨�������Ľ�ɫ������
		��̨��������Ϊǰ̨������������һ֡�Ļ��棬
		��ǰ̨��������Ϊ��չʾ��������һ֡תΪ��̨���������ȴ�������ݡ�
		���ǰѽ������������е����������֮Ϊ��ȾĿ����ͼ��RTV�����������ֶѡ���Դ������һ���ɸ�ǩ���󶨵���Ⱦ���ߣ�
		ǰ��̨��������ֻ���������Ϊ���֣�presenting���ύ����������һ�ָ�Ч�Ĳ�����
		ֻ�轻��ָ��ǰǰ̨�������ͺ�̨������������ָ�뼴��ʵ�֡�
		ǰ̨�������ͺ�̨���������ཻ����ʾ�����˽�������swap chain����
		Direct3D����IDXGISwapChain�ӿڱ�ʾ������ӿڲ����洢��ǰ̨�������ͺ�̨��������������
		���һ��ṩ���޸Ļ�������С��IDXGISwapChain::ResizeBuffers���ͳ��ֻ��������ݣ�IDXGISwapChain::Present���ķ���
		ʹ��������������ǰ̨�ͺ�̨���������Ϊ˫���壨double buffering��Ҳ�н�˫�ػ��塢˫�����壩��
		Ҳ�������ø���Ļ����������磬ʹ��3���������ͽ������ػ��壨��������ȣ�
	*/
	ComPtr <IDXGISwapChain> swapChain;//����������
	static const UINT swapChainBufferCount=2;//����������������������������Ҫ��2������������Ϊ��2�����ܻ��ཻ��
	UINT currentSwapChainBufferNO=0;//��ǰʹ�õĽ��������������
	ComPtr < ID3D12DescriptorHeap> rtvHeap;//RTV��������		Render Target View����ȾĿ����ͼ��
	ComPtr < ID3D12Resource> swapChainBufferResource[swapChainBufferCount];//��Դ��D3D��Ⱦ���ݸ�ʽ�����綥�����ݡ���������
	UINT rtvHeapSize;
	/*
	* ���賡������һЩ��͸�������壬��ô�����������������ϵĵ����ڵ�ס������һ�������ϵĶ�Ӧ��
	��Ȼ������һ������ȷ���ڳ�����������������ļ�����ͨ�����ּ��������ǾͲ����ٵ��ĳ���������Ļ���˳���ˡ�
	*/
	ComPtr < ID3D12DescriptorHeap> dsvHeap;//DSV��������		Depth Stencil View����Ȼ�����ͼ��
	ComPtr < ID3D12Resource> dsvResource;
	/*
		������������constant buffer����Ҳ��һ��GPU��Դ��ID3D12Resource��
		���������ݿɹ���ɫ�����������á�ͨ����CPUÿ֡����һ�Ρ�
		����������ڳ����в����ƶ�����ôGPU���õ���Դ����Ҫ���ϸ�����Ⱦ
		������Դ��ӦΪCBV��Դ���������ϴ���upload buffer��Դ��Ҳ������CPU�����µ���ͼ����������ϴ���GPU����Դ
	*/
	ComPtr < ID3D12DescriptorHeap> cbvHeap;//CBV��������		Constant Buffer View������������ͼ��
	ComPtr<ID3D12Resource> cbvResource;//CBVʹ�õ��ϴ���
	ComPtr<ID3D12DescriptorHeap> srvHeap;//SRV��������		Shader Resource View����ɫ����Դ��ͼ��
	
	/*
		��ǩ����root signature�������壺��ִ�л�������֮ǰ����ЩӦ�ó��򽫰󶨵���Ⱦ��ˮ���ϵ���Դ
		���ǻᱻӳ�䵽��ɫ���Ķ�Ӧ����Ĵ�����
		�������ɫ��������һ������������������Դ������ɫ���ĺ�����������ô��ǩ�������˺���ǩ����
		�� PSO ������֮ʱ�𣬸�ǩ������ɫ���������ϾͿ�ʼ��Ч
		D3D�У���ǩ����ID3D12RootSignature�ӿ�����ʾ������һ���������Ƶ��ù�������ɫ��������Դ�ĸ�������root parameter��������ɡ�
		������������ ��������root constant��������������root descriptor��������������descriptor table��
		��������ָ���������������д�����������һ����������
		�ڴ�����Ҫͨ����д CD3DX12_ROOT_PARAMETER �ṹ��������������
	*/
	ComPtr < ID3D12RootSignature> rootSignature;//��ǩ������
	ComPtr<ID3DBlob> vsData, psData;//��ɫ�����ݣ�һ����VS��Vertex Shader��������ɫ����һ����PS��Pixel Shader��������ɫ��
	/*
		��ˮ��״̬����Pipeline State Object��PSO�������������ͼ����ˮ��״̬�Ķ���ͳ��Ϊ��ˮ��״̬����
		�� ID3D12PipelineState �ӿڱ�ʾ��Ҫ����PSO����������Ҫ��дһ��������ϸ�ڵ� D3D12_GRAPHICS_PIPELINE_STATE_DESC �ṹ��ʵ��
		�����һ��PSO�������б���󶨣���ô��������������һ�� PSO �����������б�֮ǰ����һֱ���õ�ǰ�� PSO �������塣
	*/
	ComPtr < ID3D12PipelineState> pipelineState;//��Ⱦ����״̬����
	/*
		���ӣ�CPU�������������ӻ���Cubeλ����ϢP1������A1�����������������������������CPU
		����CPU�����ִ�к���ָ���GPUִ�л�������A1֮ǰ�����CPU���ȸ�д�����ݣ���ǰ�����е�λ����Ϣ�޸�ΪP2
		��ô�����Ϊ�ͻ����һ�����صĴ�����ʱ�����ǰ�ԭ������ƣ������ڻ��ƵĹ����и�����Դ���Ǵ������Ϊ��
		����������һ�ְ취�ǣ�ǿ��CPU�ȴ���ֱ��GPU�����������Ĵ����ﵽĳ��ָ����Χ���㣨fence point��Ϊֹ
		���ǽ����ַ�����Ϊˢ��������У�flush command queue��������ͨ��Χ����fence����ʵ����һ��
		Χ���� ID3D12Fence �ӿ�����ʾ���˼���������ʵ��GPU��CPU���ͬ����
		ǿ��CPU�ȴ�ֱ��GPU�����������Ĵ����ﵽĳ��ָ����Χ����fence point
	*/
	ComPtr<ID3D12Fence>					fence;//Χ��
	UINT64 currentFenceValue=0;//Χ�����ֵ
	void flushCommandQueue();//ˢ���������
	
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



	D3D12_DESCRIPTOR_HEAP_DESC createD3D12_DESCRIPTOR_HEAP_DESC(//������������
		D3D12_DESCRIPTOR_HEAP_TYPE Type= D3D12_DESCRIPTOR_HEAP_TYPE_RTV//�ѵ�����
		,UINT NumDescriptors= swapChainBufferCount//���е������������������RTV�ѣ�Ӧ��Ϊ����������������
		,D3D12_DESCRIPTOR_HEAP_FLAGS Flags= D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		/*
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		*/
		,UINT NodeMask=0//���ڵ���������������������Ϊ0������ж���������ڵ㣬������һ��λ����ʶ��������Ӧ�õ��Ľڵ㣨�豸������������֮һ���������е�ÿһλ��Ӧһ���ڵ㡣ֻ������һλ������Ķ�������ϵͳ��
	);

	/*
		�ѣ�Heap����GPU��Դ�����ڶ��У��䱾���Ǿ����ض����Ե� GPU �Դ�顣
		Ϊ��ʹ���ܴﵽ��ѣ�ͨ��Ӧ����Դ������Ĭ�϶��С�ֻ������Ҫʹ���ϴ��ѻ�ض��ѵ�����֮ʱ��ѡ���������͵Ķѡ�
		���ھ�̬�����壨static geometry����ÿһ֡�����ᷢ���ı�ļ����壩����
		���ǻὫ�䶥�㻺������Ĭ�϶ѣ�D3D12_HEAP_TYPE_DEFAULT�������Ż����ܡ�
	*/
	D3D12_HEAP_PROPERTIES createD3D12_HEAP_PROPERTIES(//������
		D3D12_HEAP_TYPE Type= D3D12_HEAP_TYPE_DEFAULT
		/*
			D3D12_HEAP_TYPE_DEFAULT		Ĭ�϶ѵ���Դֻ����GPU����
			D3D12_HEAP_TYPE_UPLOAD		��CPU�ϴ���GPU����Դ�������ľ��Ǵ���Դ�ھ���CPU�ļ����޸ĺ��ϴ���GPU�ٰ󶨵���Ⱦ����
			D3D12_HEAP_TYPE_READBACK	��GPU �������ݽ����Ż��� CPU ���ʣ��˶��е���Դ����ʹ��D3D12_RESOURCE_STATE _COPY_DEST �����������޷����ġ�
			D3D12_HEAP_TYPE_CUSTOM		ָ���Զ����
		*/
		,D3D12_CPU_PAGE_PROPERTY CPUPageProperty= D3D12_CPU_PAGE_PROPERTY_UNKNOWN
		,D3D12_MEMORY_POOL MemoryPoolPreference= D3D12_MEMORY_POOL_UNKNOWN
		,UINT CreationNodeMask=1
		,UINT VisibleNodeMask=1
	);

	D3D12_RESOURCE_DESC createD3D12_RESOURCE_DESC(//������Դ
		UINT64 Width
		,UINT Height
		,D3D12_RESOURCE_DIMENSION Dimension= D3D12_RESOURCE_DIMENSION_UNKNOWN
		/*
			D3D12_RESOURCE_DIMENSION_UNKNOWN ��Դ����δ֪��
			D3D12_RESOURCE_DIMENSION_BUFFER ��Դ��һ����������
			D3D12_RESOURCE_DIMENSION_TEXTURE1D ��Դ��һά����
			D3D12_RESOURCE_DIMENSION_TEXTURE2D ��Դ�� 2D ����
			D3D12_RESOURCE_DIMENSION_TEXTURE3D ��Դ�� 3D ����
		*/
		,UINT64 Alignment=0
		,UINT16 DepthOrArraySize=1//ָ����Դ����ȣ������ 3D���������С��������� 1D �� 2D ��Դ�����飩��
		,UINT16 MipLevels=1//ָ�� MIP �����������
		,DXGI_FORMAT Format= DXGI_FORMAT_UNKNOWN
		, UINT Count=1
		,UINT Quality=0
		,D3D12_TEXTURE_LAYOUT Layout= D3D12_TEXTURE_LAYOUT_UNKNOWN//��ʾ����������������˳����ʱ��Ϊ���������˳�򡱣��洢��
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
	* Ϊ��ʵ�ֳ�������ȾЧ����GPUͨ������һ��������д����ԴR��Ȼ���ں���Ĳ����ж�ȡ��ԴR������
	���GPUû����ɶ���Դ��д�룬���߸���û�п�ʼд�룬��ô����Դ��ȡ����һ����Դ���ա�Ϊ�˽���������
	Direct3D��һ��״̬��������Դ����Դ�ڴ���ʱ����Ĭ��״̬����ȡ����Ӧ�ó������Direct3D�κ�״̬ת��
	��ʹGPU�ܹ����κ�����Ҫ���Ĺ���������ɹ��ɺͷ�ֹ��ԴΣ�������磬�������д��һ����Դ������һ������
	���ǽ�����״̬����Ϊ��ȾĿ��״̬;��������Ҫ��ȡ����ʱ�����ǽ���״̬����Ϊ��ɫ����Դ״̬��ͨ��֪ͨDirect3Dת��
	GPU���Բ�ȡһЩ��ʩ��������գ�����ȴ����е�д������ɺ��ٴ���Դ�ж�ȡ����������ԭ��
	��Դת���ĸ���������Ӧ�ó��򿪷���Ա���ϡ�Ӧ�ó��򿪷���Ա֪����Щת����ʱ�������Զ����ɸ���ϵͳ�����Ӷ���Ŀ�����
	* ת����Դ���ϣ�transition resource barrier����ͨ�������б�����ת����Դ��������
		����ָ����Դ��ת�����������ȾĿ��״̬ת��Ϊ��ɫ����Դ״̬��
	*/
	D3D12_RESOURCE_BARRIER createD3D12_RESOURCE_BARRIER(//������Դ����
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
		//ע�������ƶ�ָ�벢���ǵ���һ��RTV	���Ǵ�RTV�Ķѿ�ʼ����ǰ��������ƫ��
		return rtvHeapCPUHandle;
	}
	
	//��ʼ��Ĭ���������λ��
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
		D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };//��ͼ�˿ڶ���
	D3D12_RECT	 rect = { 0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
	POINT lastRecordMousePosition;
	bool isLeftDown = false;
	bool isRightDown = false;
	BYTE* mMappedData;
	

	//������ Vector Position��Point��һ����˼
	XMVECTOR cameraPosition = XMVectorSet(0.f, 0.f, -10.f, 1.0f);
	XMVECTOR lookAtPoint = XMVectorZero();//����
	float angle = 180.f;//cameraPosition���lookAtPoint�ĽǶ�
	float radian= XM_PI;

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;
	

	float get_cameraPosition_lookAtPoint_between_StraightDistance();//����õ�cameraPosition��lookAtPoint֮���3Dֱ�߾���
	
	MeshGeometry cube;
	D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }//�����벼�ָ���IA�����㻺�����е�ÿ�����㶼��һ��Ԫ�أ���Ԫ��Ӧ�󶨵�������ɫ���еġ�POSITION������������˵��Ԫ�شӶ���ĵ�һ���ֽڣ��ڶ�������Ϊ 0����ʼ������ 3 ����������ÿ��������Ϊ 32 λ�� 4 ���ֽڣ�������������DXGI_FORMAT_R32G32B32_FLOAT��
		,{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		//��һ�� POSITION Ԫ��ռ12 ���ֽڣ�����������Ҫ�� POSITION ֮��Ҳ���Ǵӵ�12���ֽڿ�ʼ���� COLOR�������Ϊʲô����������� 12��
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
		L"�Կ����ƣ�%ls"
		L"\n����������꣨ % .2f, % .2f, % .2f, % .2f ��"
		L"\n����������꣨ % .2f, % .2f, % .2f, % .2f ��"
		L"\n����뽹��֮���ֱ�߾��� % .4f"
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
	ComPtr<IWICImagingFactory>			wicFactory;//wic����
	ComPtr<IWICBitmapDecoder>			wicDecoder;//����������
	ComPtr<IWICBitmapSource>			bmpSource;
	UINT bmpRowByte;
	ComPtr<ID3D12Heap>					pITextureHeap;
	ComPtr<ID3D12Heap>					pIUploadHeap;
	ComPtr<ID3D12Resource>				pITexture;
	ComPtr<ID3D12Resource>				pITextureUpload;
	UINT64 n64UploadBufferSize;
};