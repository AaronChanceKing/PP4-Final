// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include "shaders.h"
#include "h2bParser.h"
#include "level_Renderer.h"

#define PI 3.14159265359f
#define TO_RADIANS PI / 180.0f
#define MAX_SUBMESH_PER_DRAW 1024
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
	struct SHADER_MODEL_DATA
	{
		GW::MATH::GVECTORF ligthDirection, sunColor;
		GW::MATH::GMATRIXF viewMatrix, projectionMartix;
		GW::MATH::GMATRIXF matrices[MAX_SUBMESH_PER_DRAW];
		H2B::ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];

		GW::MATH::GVECTORF sunAmbient, camPos;
	};
	//GameLevel.txt
	std::vector<LevelRenderer*> OBJMESHES;

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

	// Buffers
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;
	std::vector<VkBuffer> storageHandle;
	std::vector<VkDeviceMemory> storageData;

	//Shaders
	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;

	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkDescriptorSetLayout descriptorLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSet;

	//Matrix
	GW::MATH::GMATRIXF vMatrix;
	GW::MATH::GMATRIXF pMatrix;
	//Lighting
	GW::MATH::GVECTORF LightDirection = { -1.0f, -1.0f, 2.0f, 0.0f };
	GW::MATH::GVECTORF LightColor = { 0.9f, 0.9, 1.0f, 1.0f };

	//H2B
	SHADER_MODEL_DATA ModelData;
	std::vector<GW::MATH::GMATRIXF> marixs;
	std::vector<unsigned int> mat_ID;
	std::vector<H2B::ATTRIBUTES> attrib;

public:

	struct MESH_INDEX
	{
		GW::MATH::GMATRIXF wMatrix;
		unsigned ID;
	};

	std::string levelName = "Dungeon";

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

		//View Matrix
		GW::MATH::GVECTORF eye = { 5.0f, 15.0f, -3.0f, 0.0f };
		GW::MATH::GVECTORF at = { 1.0f, 0.0f, 0.0f, 0.0f };
		GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f, 0.0f };
		proxy.LookAtLHF(eye, at, up, vMatrix);
		ModelData.viewMatrix = vMatrix;
		//Inverse View Matrix
		//proxy.InverseF(vMatrix, vMatrix);
		
		//Projection Matrix
		float AR = 0.0f;
		vlk.GetAspectRatio(AR);
		proxy.ProjectionVulkanLHF(G2D_DEGREE_TO_RADIAN(65.0f), AR, 0.1f, 100.0f, pMatrix);
		ModelData.projectionMartix = pMatrix;

		//Lighting
		float lightVectLength = std::sqrtf
		(
			(LightDirection.x * LightDirection.x) +
			(LightDirection.y * LightDirection.y) +
			(LightDirection.z * LightDirection.z)
		);
		LightDirection.x /= lightVectLength;
		LightDirection.y /= lightVectLength;
		LightDirection.z /= lightVectLength;
		ModelData.ligthDirection = LightDirection;
		ModelData.sunColor = LightColor;

		ModelData.camPos = vMatrix.row4;
		ModelData.sunAmbient = { 0.75f, 0.75f, 0.85f, 1.0f };

		//Get mesh name and send it to LevelRenderer
		std::string address = "../../Assets/" + levelName + "/GameData/GameLevel.txt";
		std::string line;
		std::fstream myFile(address);
		if (myFile.is_open())
		{
			//Read tho GameLevel.txt
			while (std::getline(myFile, line))
			{
				//Find first mesh
				if (line == "MESH")
				{
					while (std::getline(myFile, line))
					{
						GW::MATH::GMATRIXF objMatr;
						std::string mesh;
						mesh += line;
						LevelRenderer* instance = new LevelRenderer(mesh, levelName);
						OBJMESHES.push_back(instance);
						break;
					}
				}
			}
			//Close file
			myFile.close();
		}
		else std::cout << "File failed to open: " << address << std::endl;

		//Get mesh count
		int meshcount = 0;
		for (unsigned i = 0; i < OBJMESHES.size(); i++)
		{
			for (unsigned j = 0; j < OBJMESHES[i]->meshCount; j++)
			{
				ModelData.matrices[meshcount] = OBJMESHES[i]->wMatrix;
				meshcount++;
			}
		}

		//Set materials
		for (unsigned i = 0; i < OBJMESHES.size(); i++)
		{
			for (unsigned j = 0; j < OBJMESHES[i]->meshCount; j++)
			{
				attrib.push_back(OBJMESHES[i]->materials[j].attrib);
			}
		}

		for (unsigned i = 0; i < attrib.size(); i++)
		{
			ModelData.materials[i] = attrib[i];
		}

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		//Send data to buffer
		for (unsigned i = 0; i < OBJMESHES.size(); i++)
		{
			GvkHelper::create_buffer(physicalDevice, device, sizeof(H2B::VERTEX) * OBJMESHES[i]->vertexCount,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &OBJMESHES[i]->vertexHandle, &OBJMESHES[i]->vertexData);
			GvkHelper::write_to_buffer(device, OBJMESHES[i]->vertexData, OBJMESHES[i]->meshVert, sizeof(H2B::VERTEX) * OBJMESHES[i]->vertexCount);

			GvkHelper::create_buffer(physicalDevice, device, sizeof(int) * OBJMESHES[i]->indexCount,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &OBJMESHES[i]->indexHandle, &OBJMESHES[i]->indexData);
			GvkHelper::write_to_buffer(device, OBJMESHES[i]->indexData, OBJMESHES[i]->meshIndex, sizeof(int) * OBJMESHES[i]->indexCount);
		}

		unsigned int frames;
		vlk.GetSwapchainImageCount(frames);
		storageData.resize(frames);
		storageHandle.resize(frames);
		descriptorSet.resize(frames);
		for (size_t i = 0; i < frames; i++)
		{
			GvkHelper::create_buffer(physicalDevice, device, sizeof(ModelData),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &storageHandle[i], &storageData[i]);
			GvkHelper::write_to_buffer(device, storageData[i], &ModelData, sizeof(ModelData));
		}

		/***************** SHADER INTIALIZATION ******************/
				// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
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
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable = false;

		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(H2B::VERTEX);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription vertex_attribute_description[3] = 
		{
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, //uv, normal, etc....
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(H2B::VERTEX, uvw)},
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(H2B::VERTEX, nrm)}
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 3;
		input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;
		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport = 
		{
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
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
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
		VkDynamicState dynamic_state[2] = 
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 2;
		dynamic_create_info.pDynamicStates = dynamic_state;

		VkDescriptorSetLayoutBinding descriptor_layout_binding = {};
		descriptor_layout_binding.binding = 0;
		descriptor_layout_binding.descriptorCount = 1;
		descriptor_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptor_layout_binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = {};
		descriptor_layout_create_info.bindingCount = 1;
		descriptor_layout_create_info.flags = 0;
		descriptor_layout_create_info.pBindings = &descriptor_layout_binding;
		descriptor_layout_create_info.pNext = nullptr;
		descriptor_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		VkResult r = vkCreateDescriptorSetLayout(device, &descriptor_layout_create_info,
			nullptr, &descriptorLayout);

		VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
		VkDescriptorPoolSize descriptor_pool_size = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frames };
		descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptor_pool_create_info.poolSizeCount = 1;
		descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
		descriptor_pool_create_info.maxSets = frames;
		descriptor_pool_create_info.pNext = nullptr;
		vkCreateDescriptorPool(device, &descriptor_pool_create_info, nullptr, &descriptorPool);

		VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
		descriptor_set_allocate_info.descriptorPool = descriptorPool;
		descriptor_set_allocate_info.descriptorSetCount = 1;
		descriptor_set_allocate_info.pNext = nullptr;
		descriptor_set_allocate_info.pSetLayouts = &descriptorLayout;
		descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		for (unsigned i = 0; i < frames; ++i) 
		{
			vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptorSet[i]);
		}

		for (unsigned i = 0; i < frames; ++i) 
		{
			VkDescriptorBufferInfo dbinfo = { storageHandle[i], 0, VK_WHOLE_SIZE };
			VkWriteDescriptorSet write_descriptor_set = {};
			write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_descriptor_set.descriptorCount = 1;
			write_descriptor_set.dstArrayElement = 0;
			write_descriptor_set.dstBinding = 0;
			write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write_descriptor_set.dstSet = descriptorSet[i];
			write_descriptor_set.pBufferInfo = &dbinfo;
			vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
		}

		VkDescriptorBufferInfo descriptor_buffer_info = {};

		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipeline_layout_create_info.pSetLayouts = &descriptorLayout;
		pipeline_layout_create_info.setLayoutCount = 1;

		VkPushConstantRange push_constant_range = {};
		push_constant_range.offset = 0;
		push_constant_range.size = sizeof(MESH_INDEX);
		push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		pipeline_layout_create_info.pushConstantRangeCount = 1;
		pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
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
		//Update the camera
		ModelData.viewMatrix = vMatrix;
		ModelData.camPos = vMatrix.row4;

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

		VkDeviceSize offsets[] = { 0 };

		MESH_INDEX meshIndex;
		unsigned ID = 0;

		for (unsigned i = 0; i < OBJMESHES.size(); i++)
		{
			GvkHelper::write_to_buffer(device, storageData[currentBuffer], &ModelData, sizeof(SHADER_MODEL_DATA));
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet[currentBuffer], 0, nullptr);

			//Loop tho objmesh and draw
			for (unsigned j = 0; j < OBJMESHES[i]->meshCount; j++)
			{
				meshIndex = { OBJMESHES[i]->wMatrix, ID };
				ID++;

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &OBJMESHES[i]->vertexHandle, offsets);
				vkCmdBindIndexBuffer(commandBuffer, OBJMESHES[i]->indexHandle, 0, VK_INDEX_TYPE_UINT32);
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MESH_INDEX), &meshIndex);
				vkCmdDrawIndexed(commandBuffer, OBJMESHES[i]->meshes[j].drawInfo.indexCount, 1, OBJMESHES[i]->meshes[j].drawInfo.indexOffset, 0, 0);
			}
		}
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
		proxy.InverseF(vMatrix, camera);
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
		proxy.InverseF(camera, vMatrix);
	}
		
private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		vkDestroyBuffer(device, indexHandle, nullptr);
		vkFreeMemory(device, indexData, nullptr);
		for (unsigned i = 0; i < storageData.size(); i++) 
		{
			vkDestroyBuffer(device, storageHandle[i], nullptr);
			vkFreeMemory(device, storageData[i], nullptr);
		}
		storageData.clear();
		storageHandle.clear();

		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);

		for (unsigned i = 0; i < OBJMESHES.size(); i++)
		{
			vkDestroyBuffer(device, OBJMESHES[i]->vertexHandle, nullptr);
			vkFreeMemory(device, OBJMESHES[i]->vertexData, nullptr);

			vkDestroyBuffer(device, OBJMESHES[i]->indexHandle, nullptr);
			vkFreeMemory(device, OBJMESHES[i]->indexData, nullptr);
		}

		unsigned meshes = OBJMESHES.size();
		for (unsigned i = 0; i < meshes; ++i) delete OBJMESHES[i];
	}
};
