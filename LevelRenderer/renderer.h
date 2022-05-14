// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include "shaders.h"
#include "h2bParser.h"

#define PI 3.14159265359f
#define TO_RADIANS PI / 180.0f
#ifdef _WIN32 // must use MT platform DLL libraries on windows
	#pragma comment(lib, "shaderc_combined.lib") 
#endif
#pragma pack_matrix( row_major )   

//Shaders
const char* vertexShaderSource = Shaders::vertexShader;
const char* pixelShaderSource = Shaders::pixelShader;

// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;
	GW::MATH::GMatrix proxy;

	//Input
	GW::INPUT::GInput Input;
	GW::INPUT::GController Controller;
	float updatesPerSecond = 60;
	float cameraMoveSpeed = 0.18f;
	float lookSensitivity = 4.0f;

	//World matrix
	GW::MATH::GMATRIXF Matrix;
	//View matrix
	GW::MATH::GMATRIXF ViewMatrix;
	//Projection matrix
	GW::MATH::GMATRIXF ProjectionMatrix;

	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;


public:
	std::chrono::time_point<std::chrono::system_clock> start, end;

	// TODO: Part 1c
	struct Vertex { float x, y, z, w; };

	// TODO: Part 2b
	struct SHADER_VARS
	{
		GW::MATH::GMATRIXF wMatrix;
		GW::MATH::GMATRIXF vMatrix;
	};

		// TODO: Part 2f
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		//Create proxys
		Input.Create(win);
		Controller.Create();
		proxy.Create();

		//Floor
		GW::MATH::GMATRIXF matrix = GW::MATH::GIdentityMatrixF;
		GW::MATH::GMatrix::TranslateGlobalF(matrix, GW::MATH::GVECTORF({ 0.0f, -0.5f}), matrix);
		GW::MATH::GMatrix::RotateXGlobalF(matrix, G_DEGREE_TO_RADIAN(90), matrix);

		Matrix = matrix;

		// Init camera and view
		GW::MATH::GVECTORF eye = { 2.0f, 0.5f, 0.0f };
		GW::MATH::GVECTORF at = { 0.0f, 0.0f, 0.0f };
		GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f };
		proxy.LookAtLHF(eye, at, up, ViewMatrix);

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		// TODO: Part 1b
		// TODO: Part 1c
		/*Vertex verts[] = {
			{0.0f,   0.5f, 0.0f, 1.0f},
			{0.5f, -0.5f, 0.0f, 1.0f},
			{-0.5f, -0.5f, 0.0f, 1.0f}
		};*/
		// Create Vertex Buffer
		/*float verts[] = {
			 0.0f,   0.5f,
			 0.5f, -0.5f,
			-0.5f, -0.5f
		};*/
		// TODO: Part 1d
		Vertex verts[] = {
			{	-0.50f,	 0.50f,		0,		1	},
			{	 0.50f,	 0.50f,		0,		1	},
			{	-0.5f,	 0.46f,		0,		1	},
			{	 0.5f,	 0.46f,		0,		1	},
			{	-0.5f,	 0.42f,		0,		1	},
			{	 0.5f,	 0.42f,		0,		1	},
			{	-0.5f,	 0.38f,		0,		1	},
			{	 0.5f,	 0.38f,		0,		1	},
			{	-0.5f,	 0.34f,		0,		1	},
			{	 0.5f,	 0.34f,		0,		1	},
			{	-0.5f,	 0.30f,		0,		1	},
			{	 0.5f,	 0.30f,		0,		1	},
			{	-0.5f,	 0.26f,		0,		1	},
			{	 0.5f,	 0.26f,		0,		1	},
			{	-0.5f,	 0.22f,		0,		1	},
			{	 0.5f,	 0.22f,		0,		1	},
			{	-0.5f,	 0.18f,		0,		1	},
			{	 0.5f,	 0.18f,		0,		1	},
			{	-0.5f,	 0.14f,		0,		1	},
			{	 0.5f,	 0.14f,		0,		1	},
			{	-0.5f,	 0.10f,		0,		1	},
			{	 0.5f,	 0.10f,		0,		1	},
			{	-0.5f,	 0.06f,		0,		1	},
			{	 0.5f,	 0.06f,		0,		1	},
			{	-0.5f,	 0.02f,		0,		1	},
			{	 0.5f,	 0.02f,		0,		1	},

			{	-0.50f,	 -0.50f,	0,		1	},
			{	 0.50f,	 -0.50f,	0,		1	},
			{	-0.5f,	 -0.46f,	0,		1	},
			{	 0.5f,	 -0.46f,	0,		1	},
			{	-0.5f,	 -0.42f,	0,		1	},
			{	 0.5f,	 -0.42f,	0,		1	},
			{	-0.5f,	 -0.38f,	0,		1	},
			{	 0.5f,	 -0.38f,	0,		1	},
			{	-0.5f,	 -0.34f,	0,		1	},
			{	 0.5f,	 -0.34f,	0,		1	},
			{	-0.5f,	 -0.30f,	0,		1	},
			{	 0.5f,	 -0.30f,	0,		1	},
			{	-0.5f,	 -0.26f,	0,		1	},
			{	 0.5f,	 -0.26f,	0,		1	},
			{	-0.5f,	 -0.22f,	0,		1	},
			{	 0.5f,	 -0.22f,	0,		1	},
			{	-0.5f,	 -0.18f,	0,		1	},
			{	 0.5f,	 -0.18f,	0,		1	},
			{	-0.5f,	 -0.14f,	0,		1	},
			{	 0.5f,	 -0.14f,	0,		1	},
			{	-0.5f,	 -0.10f,	0,		1	},
			{	 0.5f,	 -0.10f,	0,		1	},
			{	-0.5f,	 -0.06f,	0,		1	},
			{	 0.5f,	 -0.06f,	0,		1	},
			{	-0.5f,	 -0.02f,	0,		1	},
			{	 0.5f,	 -0.02f,	0,		1	},




			{	0.50f,	-0.50f,	 	0,		1	},
			{	0.50f,	 0.50f,	 	0,		1	},
			{	0.46f,	-0.5f,	 	0,		1	},
			{	0.46f,	 0.5f,	 	0,		1	},
			{	0.42f,	-0.5f,		0,		1	},
			{	0.42f,	 0.5f,	 	0,		1	},
			{	0.38f,	-0.5f,	 	0,		1	},
			{	0.38f,	 0.5f,	 	0,		1	},
			{	0.34f,	-0.5f,	 	0,		1	},
			{	0.34f,	 0.5f,	 	0,		1	},
			{	0.30f,	-0.5f,	 	0,		1	},
			{	0.30f,	 0.5f,	 	0,		1	},
			{	0.26f,	-0.5f,	 	0,		1	},
			{	0.26f,	 0.5f,	 	0,		1	},
			{	0.22f,	-0.5f,	 	0,		1	},
			{	0.22f,	 0.5f,	 	0,		1	},
			{	0.18f,	-0.5f,	 	0,		1	},
			{	0.18f,	 0.5f,	 	0,		1	},
			{	0.14f,	-0.5f,	 	0,		1	},
			{	0.14f,	 0.5f,	 	0,		1	},
			{	0.10f,	-0.5f,	 	0,		1	},
			{	0.10f,	 0.5f,	 	0,		1	},
			{	0.06f,	-0.5f,	 	0,		1	},
			{	0.06f,	 0.5f,	 	0,		1	},
			{	0.02f,	-0.5f,	 	0,		1	},
			{	0.02f,	 0.5f,	 	0,		1	},

			{	-0.50f,	-0.50f,	 	0,		1	},
			{	-0.50f,	 0.50f,	 	0,		1	},
			{	-0.46f,	-0.5f,	 	0,		1	},
			{	-0.46f,	 0.5f,	 	0,		1	},
			{	-0.42f,	-0.5f,		0,		1	},
			{	-0.42f,	 0.5f,	 	0,		1	},
			{	-0.38f,	-0.5f,	 	0,		1	},
			{	-0.38f,	 0.5f,	 	0,		1	},
			{	-0.34f,	-0.5f,	 	0,		1	},
			{	-0.34f,	 0.5f,	 	0,		1	},
			{	-0.30f,	-0.5f,	 	0,		1	},
			{	-0.30f,	 0.5f,	 	0,		1	},
			{	-0.26f,	-0.5f,	 	0,		1	},
			{	-0.26f,	 0.5f,	 	0,		1	},
			{	-0.22f,	-0.5f,	 	0,		1	},
			{	-0.22f,	 0.5f,	 	0,		1	},
			{	-0.18f,	-0.5f,	 	0,		1	},
			{	-0.18f,	 0.5f,	 	0,		1	},
			{	-0.14f,	-0.5f,	 	0,		1	},
			{	-0.14f,	 0.5f,	 	0,		1	},
			{	-0.10f,	-0.5f,	 	0,		1	},
			{	-0.10f,	 0.5f,	 	0,		1	},
			{	-0.06f,	-0.5f,	 	0,		1	},
			{	-0.06f,	 0.5f,	 	0,		1	},
			{	-0.02f,	-0.5f,	 	0,		1	},
			{	-0.02f,	 0.5f,	 	0,		1	},
		};


		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		GvkHelper::create_buffer(physicalDevice, device, sizeof(verts),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, verts, sizeof(verts));

		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		// TODO: Part 3C
		shaderc_compile_options_set_invert_y(options, false);
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource, strlen(vertexShaderSource),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, pixelShaderSource, strlen(pixelShaderSource),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &pixelShader);
		shaderc_result_release(result); // done
		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";
		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = pixelShader;
		stage_create_info[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; /////
		assembly_create_info.primitiveRestartEnable = false;
		// Vertex Input State
		// TODO: Part 1c
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(Vertex);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		// TODO: Part 1c
		VkVertexInputAttributeDescription vertex_attribute_description[1] = {
			{ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 } //uv, normal, etc....
		};

		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 1;
		input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;
		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport = {
            0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
        };
        VkRect2D scissor = { {0, 0}, {width, height} };
		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount = 1;
		viewport_create_info.pViewports = &viewport;
		viewport_create_info.scissorCount = 1;
		viewport_create_info.pScissors = &scissor;
		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_LINE;// VK_POLYGON_MODE_FILL;
		rasterization_create_info.lineWidth = 1.0f;
		rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info.depthClampEnable = VK_FALSE;
		rasterization_create_info.depthBiasEnable = VK_FALSE;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;
		// Multisampling State
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable = VK_FALSE;
		multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading = 1.0f;
		multisample_create_info.pSampleMask = VK_NULL_HANDLE;
		multisample_create_info.alphaToCoverageEnable = VK_FALSE;
		multisample_create_info.alphaToOneEnable = VK_FALSE;
		// Depth-Stencil State
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
		depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_create_info.minDepthBounds = 0.0f;
		depth_stencil_create_info.maxDepthBounds = 1.0f;
		depth_stencil_create_info.stencilTestEnable = VK_FALSE;
		// Color Blending Attachment & State
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
		color_blend_attachment_state.colorWriteMask = 0xF;
		color_blend_attachment_state.blendEnable = VK_FALSE;
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment_state;
		color_blend_create_info.blendConstants[0] = 0.0f;
		color_blend_create_info.blendConstants[1] = 0.0f;
		color_blend_create_info.blendConstants[2] = 0.0f;
		color_blend_create_info.blendConstants[3] = 0.0f;
		// Dynamic State 
		VkDynamicState dynamic_state[2] = { 
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 2;
		dynamic_create_info.pDynamicStates = dynamic_state;

		// TODO: Part 2c
		VkPushConstantRange pushConstrantRange;
		pushConstrantRange.offset = 0;
		pushConstrantRange.size = sizeof(SHADER_VARS);
		pushConstrantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 0;
		pipeline_layout_create_info.pSetLayouts = VK_NULL_HANDLE;
		pipeline_layout_create_info.pushConstantRangeCount = 1; // TODO: Part 2d 
		pipeline_layout_create_info.pPushConstantRanges = &pushConstrantRange; // TODO: Part 2d
		vkCreatePipelineLayout(device, &pipeline_layout_create_info, 
			nullptr, &pipelineLayout);
	    // Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, 
			&pipeline_create_info, nullptr, &pipeline);

		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
		});
	}
	void Render()
	{
		// grab the current Vulkan commandBuffer
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);
		// what is the current client area dimensions?
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// setup the pipeline's dynamic settings
		VkViewport viewport = {
            0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
        };
        VkRect2D scissor = { {0, 0}, {width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// TODO: Part 3a

		//Might need to turn this back on and try to get it working
		//
		/*GW::GRAPHICS::GVulkanSurface surface = {};
		float aspect;
		surface.GetAspectRatio(aspect);*/

		GW::MATH::GMatrix::ProjectionVulkanLHF(G_DEGREE_TO_RADIAN(65.0f), 800.0f/600.0f, 0.1f, 100.0f, ProjectionMatrix);  //Possibly going to fuck something up(param 2 see above)
		
		// TODO: Part 3b
		// TODO: Part 2b
		SHADER_VARS wMatrix;
		wMatrix.wMatrix = Matrix;
		
		// TODO: Part 2f, Part 3b
		GW::MATH::GMATRIXF combo;
		GW::MATH::GMatrix::MultiplyMatrixF(ViewMatrix, ProjectionMatrix, combo);
		wMatrix.vMatrix = combo;
		// TODO: Part 2d
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SHADER_VARS), &wMatrix);
		
		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		vkCmdDraw(commandBuffer, 104, 1, 0, 0); // TODO: Part 1b // TODO: Part 1c
		
	}

	//Camera movment
	void UpdateCamera()
	{
		// Timer
		static std::chrono::system_clock::time_point prevTime = std::chrono::system_clock::now();
		std::chrono::duration<float> deltaTime = std::chrono::system_clock::now() - prevTime;
		if (deltaTime.count() >= 1.0f / updatesPerSecond) {
			prevTime += std::chrono::system_clock::now() - prevTime;
		}

		//Set Camera to inverse of view
		GW::MATH::GMATRIXF camera;
		proxy.InverseF(ViewMatrix, camera);
		GW::MATH::GVECTORF displacement;

		//Get Inputs
		float SpaceBar = 0;		Input.GetState(G_KEY_SPACE, SpaceBar);
		float LShift = 0;		Input.GetState(G_KEY_LEFTSHIFT, LShift);
		float RTrigger = 0;		Input.GetState(G_RIGHT_TRIGGER_AXIS, RTrigger);
		float LTrigger = 0;		Input.GetState(G_LEFT_TRIGGER_AXIS, LTrigger);
		float W = 0;			Input.GetState(G_KEY_W, W);
		float A = 0;			Input.GetState(G_KEY_A, A);
		float S = 0;			Input.GetState(G_KEY_S, S);
		float D = 0;			Input.GetState(G_KEY_D, D);
		float LStickY = 0;		Input.GetState(G_LY_AXIS, LStickY);
		float LStickX = 0;		Input.GetState(G_LX_AXIS, LStickX);

		displacement = {
			(D - A + LStickX) * deltaTime.count() * cameraMoveSpeed,
			0,
			(W - S + LStickY) * deltaTime.count() * cameraMoveSpeed 
		};

		proxy.TranslateLocalF(camera, displacement, camera);

		displacement = { 0, (SpaceBar - LShift + RTrigger - LTrigger) * deltaTime.count() * cameraMoveSpeed, 0 };
		GW::MATH::GVector::AddVectorF(camera.row4, displacement, camera.row4);

		//Get rotate
		float RStickY = 0;				Input.GetState(G_RY_AXIS, RStickY);
		float RStickX = 0;				Input.GetState(G_RX_AXIS, RStickX);

		float mouseX = 0;
		float mouseY = 0;
		GW::GReturn result = Input.GetMouseDelta(mouseX, mouseY);

		unsigned int screenHeight = 0;	win.GetHeight(screenHeight);
		unsigned int screenWidth = 0;	win.GetWidth(screenWidth);


		if (G_PASS(result) && result != GW::GReturn::REDUNDANT)
		{
			float thumbSpeed = PI * deltaTime.count();
			float pitch = (60.0f * TO_RADIANS * mouseY * lookSensitivity) / (screenHeight + RStickY * (-thumbSpeed));
			GW::MATH::GMATRIXF rotation;
			proxy.RotationYawPitchRollF(0, pitch, 0, rotation);
			proxy.MultiplyMatrixF(rotation, camera, camera);

			float yaw = 60.0f * TO_RADIANS * mouseX * lookSensitivity / screenWidth + RStickX * thumbSpeed;
			proxy.RotateYGlobalF(camera, yaw, camera);
		}

		// Apply to view matrix and inverse
		proxy.InverseF(camera, ViewMatrix);
	}
		
private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
	}
};
