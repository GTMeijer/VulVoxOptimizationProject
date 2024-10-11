#include "pch.h"
#include "vulkan_window.h"

//TODO: Probably do some lambda stuff here?
void Vulkan_Window::main_loop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        draw_frame();
    }

    //Wait until all operations are completed before cleanup
    vkDeviceWaitIdle(vulkan_instance.device);
}

void Vulkan_Window::draw_frame()
{
    vkWaitForFences(vulkan_instance.device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

    //Ask the swapchain for a render image to target
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(vulkan_instance.device, swap_chain.swap_chain, UINT64_MAX, image_available_semaphores[current_frame], nullptr, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        //Swap chain in not compatible with the current window size, recreate
        recreate_swap_chain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    //Update global variables (camera etc.)
    update_uniform_buffer(current_frame);

    //Reset fence *after* confirming the swapchain is valid (prevents deadlock)
    vkResetFences(vulkan_instance.device, 1, &in_flight_fences[current_frame]);

    //Start recording a new command buffer for rendering
    vkResetCommandBuffer(command_buffers[current_frame], 0); //Remove previous commands and free memory
    record_command_buffer(command_buffers[current_frame], image_index);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //Which semaphore to wait for before execution and at which stage
    std::array<VkSemaphore, 1> wait_semaphores = { image_available_semaphores[current_frame] };
    std::array<VkPipelineStageFlags, 1> wait_stages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = wait_semaphores.size();
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_stages.data();

    //Which semaphore to signal when the command buffer is done
    std::array<VkSemaphore, 1> signal_semaphores = { render_finished_semaphores[current_frame] };
    submit_info.signalSemaphoreCount = signal_semaphores.size();
    submit_info.pSignalSemaphores = signal_semaphores.data();

    //Link command buffer
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[current_frame];

    //Submit the command buffer so the GPU starts executing it
    if (vkQueueSubmit(vulkan_instance.graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores.data(); //Wait for semaphore before we can present

    std::array<VkSwapchainKHR, 1> swap_chains = { swap_chain.swap_chain };
    present_info.swapchainCount = swap_chains.size();
    present_info.pSwapchains = swap_chains.data();
    present_info.pImageIndices = &image_index;

    //Dont need to collect the draw results because the present function returns them as well
    present_info.pResults = nullptr;

    //Present the result on screen
    result = vkQueuePresentKHR(vulkan_instance.present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized)
    {
        framebuffer_resized = false;

        //Swap chain in not compatible with the current window size, recreate
        recreate_swap_chain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    //Rotate to next frame resources
    current_frame = (current_frame++) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan_Window::update_uniform_buffer(uint32_t current_image)
{
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    //Rotate around the z-axis
    MVP mvp{};
    mvp.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f)) * glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f));
    mvp.view = glm::lookAt(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mvp.projection = glm::perspective(glm::radians(45.0f), (float)swap_chain.extent.width / (float)swap_chain.extent.height, 0.1f, 100.0f);

    //mvp.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    //mvp.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //mvp.projection = glm::perspective(glm::radians(45.0f), (float)swap_chain.extent.width / (float)swap_chain.extent.height, 0.1f, 10.0f);

    mvp.projection[1][1] *= -1; //Invert y-axis so its compatible with Vulkan axes

    memcpy(uniform_buffers_mapped[current_image], &mvp, sizeof(mvp));
}

void Vulkan_Window::init_window()
{
    std::cout << "Init window.." << std::endl;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

    //Pass pointer of this to glfw so we can retrieve the object instance in the callback function
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

    std::cout << "Window initialized." << std::endl;
}

void Vulkan_Window::init_vulkan()
{
    std::cout << "Init vulkan.." << std::endl;

    vulkan_instance.init_instance();

    create_surface();

    vulkan_instance.init_device(surface);

    swap_chain.create_swap_chain(window, surface);

    create_render_pass();
    create_descriptor_set_layout();
    create_graphics_pipeline();
    create_command_pool();
    create_depth_resources();
    create_framebuffers();
    create_texture_image();
    create_texture_image_view();
    create_texture_sampler();

    konata_model = Model(MODEL_PATH);

    instance_konata.model = Model(MODEL_PATH);

    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();
    create_instance_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffer();
    create_sync_objects();

    std::cout << "Vulkan initialized." << std::endl;
}

void Vulkan_Window::cleanup()
{
    cleanup_swap_chain();

    vkDestroyPipeline(vulkan_instance.device, vertex_pipeline, nullptr);
    vkDestroyPipeline(vulkan_instance.device, instance_pipeline, nullptr);
    vkDestroyPipelineLayout(vulkan_instance.device, pipeline_layout, nullptr);
    vkDestroyRenderPass(vulkan_instance.device, render_pass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        uniform_buffers[i].destroy(vulkan_instance.device);
    }

    //Descriptor sets will be destroyed with the pool
    vkDestroyDescriptorPool(vulkan_instance.device, descriptor_pool, nullptr);

    //Texture cleanup
    texture_image.destroy();

    //Cleanup descriptor set layout and buffers
    vkDestroyDescriptorSetLayout(vulkan_instance.device, descriptor_set_layout, nullptr);

    index_buffer.destroy(vulkan_instance.device);
    vertex_buffer.destroy(vulkan_instance.device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(vulkan_instance.device, image_available_semaphores.at(i), nullptr);
        vkDestroySemaphore(vulkan_instance.device, render_finished_semaphores.at(i), nullptr);
        vkDestroyFence(vulkan_instance.device, in_flight_fences.at(i), nullptr);
    }

    //Also destroys the command buffers
    vkDestroyCommandPool(vulkan_instance.device, command_pool, nullptr);

    vulkan_instance.cleanup_device();

    vkDestroySurfaceKHR(vulkan_instance.instance, surface, nullptr);

    vulkan_instance.cleanup_instance();

    glfwDestroyWindow(window);

    glfwTerminate();
}

void Vulkan_Window::create_surface()
{
    if (glfwCreateWindowSurface(vulkan_instance.instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void Vulkan_Window::create_render_pass()
{
    //The render pass describes the framebuffer attachments 
    //and how many color and depth buffers there are
    //and how their content should be handled

    VkAttachmentDescription color_attachment{};
    color_attachment.format = swap_chain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; //No multisampling
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //Clear buffer before rendering
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //Store rendered content

    //No stencil buffer operations yet, so dont care about the data
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    //This buffer is used for presentation
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    //Index of fragment shader out_color
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //Describe how the depth buffer is handled
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = vulkan_instance.find_depth_format();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //We dont care what the gpu does with the data after determining the depth
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //We dont care about previous depth content
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //Attach the color and depth buffers to the single subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    //Setup the dependencies between the subpasses (cant start b before a)
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    //Wait until the swap chain finished reading before writing a new image
    //Also wait with writing a new depth buffer until the previous is done being read
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    //Attach the color and depth attachements to the render pass
    std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(vulkan_instance.device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

/// <summary>
/// Creates the descriptor sets that describe the layout of the data buffers in our pipeline.
/// Some hardware only allows up to four descriptor sets being created,
/// so instead of allocating a seperate set for each resource we bind them together into one.
/// </summary>
void Vulkan_Window::create_descriptor_set_layout()
{
    //Layout binding for the uniform buffer
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0; //Same as in shader
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //This buffer is (only) referenced in the vertex stage
    ubo_layout_binding.pImmutableSamplers = nullptr; //Optional

    //Layout binding for the image sampler
    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //Connect to fragment shader stage

    //Bind the layout bindings in the single descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vulkan_instance.device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to created descriptor set layout!");
    }
}

void Vulkan_Window::create_graphics_pipeline()
{

    //Define global variables (like a MVP matrix)
    //These are defined in a seperate pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1; //Amount of descriptor set layout (uniform buffer layout)
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0; //Optional small uniform data
    pipeline_layout_info.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(vulkan_instance.device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    //Load compiled SPIR-V shader files 
    std::filesystem::path vert_shader_filepath("shaders/vert.spv");
    std::filesystem::path frag_shader_filepath("shaders/frag.spv");

    std::filesystem::path instance_vert_shader_filepath("shaders/instance_vert.spv");
    std::filesystem::path instance_frag_shader_filepath("shaders/instance_frag.spv");

    Vulkan_Shader vert_shader{ vulkan_instance.device, vert_shader_filepath, "main", VK_SHADER_STAGE_VERTEX_BIT };
    Vulkan_Shader frag_shader{ vulkan_instance.device, frag_shader_filepath, "main", VK_SHADER_STAGE_FRAGMENT_BIT };
    Vulkan_Shader instance_vert_shader{ vulkan_instance.device, instance_vert_shader_filepath, "main", VK_SHADER_STAGE_VERTEX_BIT };
    Vulkan_Shader instance_frag_shader{ vulkan_instance.device, instance_frag_shader_filepath, "main", VK_SHADER_STAGE_FRAGMENT_BIT };

    //Describes the configuration of the vertices the triangles and lines use
    //v
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //Triangle from every three vertices, without reuse
    input_assembly_info.primitiveRestartEnable = VK_FALSE; //Break up lines and triangles in strip mode
    input_assembly_info.flags = 0;

    //Viewport size
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swap_chain.extent.width;
    viewport.height = (float)swap_chain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    //Visible part of the viewport (whole viewport)
    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = swap_chain.extent;

    //It is usefull to have a non-static viewport and scissor size
    std::vector<VkDynamicState> dynamic_states =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    //v
    VkPipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    //v
    VkPipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.flags = 0;

    //Setup rasterizer stage
    //v
    VkPipelineRasterizationStateCreateInfo rasterizer_info{};
    rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.depthClampEnable = VK_FALSE; //Discard fragments outside of near/far plane (instead of clamping)
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE; //If true, discards all geometery
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL; //Full render, switch for wireframe or points (among others)
    rasterizer_info.lineWidth = 1.0f; //wider requires wideLines GPU feature
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT; //Backface culling
    rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //Counter clockwise vertex order determines the front (we flip the y axis)
    rasterizer_info.depthBiasEnable = VK_FALSE;
    rasterizer_info.depthBiasConstantFactor = 0.0f;
    rasterizer_info.depthBiasClamp = 0.0f;
    rasterizer_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_info.flags = 0;

    //Multisample / anti-aliasing stage (disabled)
    //v
    VkPipelineMultisampleStateCreateInfo multisampling_info{};
    multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_info.minSampleShading = 1.0f; // Optional
    multisampling_info.pSampleMask = nullptr; // Optional
    multisampling_info.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling_info.alphaToOneEnable = VK_FALSE; // Optional
    multisampling_info.flags = 0;

    //Enable depth testing
    //v
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE; //Discard fragment is they fail depth test
    depth_stencil.depthWriteEnable = VK_TRUE; //Write passed depth tests to the depth buffer
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; //Write closer results to the depth buffer
    depth_stencil.back.compareOp = VK_COMPARE_OP_ALWAYS; //Back face culling
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.minDepthBounds = 0.0f; // Optional
    depth_stencil.maxDepthBounds = 1.0f; // Optional
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = {}; // Optional
    depth_stencil.back = {}; // Optional

    //Handle color blending of the fragments (for example alpha blending) (disabled)
    //v
    VkPipelineColorBlendAttachmentState color_blend_attachement_info{};
    color_blend_attachement_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachement_info.blendEnable = VK_FALSE; //Disabled
    color_blend_attachement_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
    color_blend_attachement_info.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
    color_blend_attachement_info.colorBlendOp = VK_BLEND_OP_ADD; //Optional
    color_blend_attachement_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
    color_blend_attachement_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
    color_blend_attachement_info.alphaBlendOp = VK_BLEND_OP_ADD; //Optional

    VkPipelineColorBlendStateCreateInfo color_blending_info{};
    color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_info.logicOpEnable = VK_FALSE; //Disabled
    color_blending_info.logicOp = VK_LOGIC_OP_COPY; //Optional
    color_blending_info.attachmentCount = 1;
    color_blending_info.pAttachments = &color_blend_attachement_info;
    color_blending_info.blendConstants[0] = 0.0f; //Optional
    color_blending_info.blendConstants[1] = 0.0f; //Optional
    color_blending_info.blendConstants[2] = 0.0f; //Optional
    color_blending_info.blendConstants[3] = 0.0f; //Optional


    //Some parts of the pipeline can be dynamic, we define these parts here
    //std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages_info = { vert_shader_stage_info, frag_shader_stage_info };
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages_info;

    //Describe the format of the vertex and instance data
    std::vector<VkVertexInputBindingDescription> binding_descriptions =
    {
        //Binding point 0: Mesh vertex layout description at per-vertex rate
        Vertex::get_binding_description(0),
        //Binding point 1: Instanced data at per-instance rate
        Instance_Data::get_binding_description(1)
    };

    //Vertex attribute bindings
    //Note that the shader declaration for per-vertex and per-instance attributes is the same, the different input rates are only stored in the bindings:
        //	layout (location = 0) in vec3 in_position;		Per-Vertex
        //	...
        //	layout (location = 3) in vec3 instance_position;	Per-Instance
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

    //Per-vertex attributes
    //These are advanced for each vertex fetched by the vertex shader
    for (auto& attribute_desc : Vertex::get_attribute_descriptions(0))
    {
        attribute_descriptions.push_back(attribute_desc);
    }

    //Per-Instance attributes
    //These are advanced for each instance rendered
    for (auto& attribute_desc : Instance_Data::get_attribute_descriptions(1))
    {
        attribute_descriptions.push_back(attribute_desc);
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_info{};
    vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertex_input_state_info.pVertexBindingDescriptions = binding_descriptions.data(); //spacing between data and per vertex or per instance
    vertex_input_state_info.pVertexAttributeDescriptions = attribute_descriptions.data(); //attribute type, which bindings to load, and offset

    //Combine the pipeline stages, we change the input and shader stages for the instance and vertex pipelines
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pVertexInputState = &vertex_input_state_info; //Differ for the instance and vertex rendering
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending_info;
    pipeline_info.pDynamicState = &dynamic_state_info;

    pipeline_info.stageCount = 2; //Vert & Frag shader stages
    pipeline_info.pStages = shader_stages_info.data(); //Differ for the instance and vertex rendering

    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    //This pipeline doesn't derive from another 
    //(set VK_PIPELINE_CREATE_DERIVATIVE_BIT, if we want to derive from another)    
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.basePipelineIndex = -1;

    ///Per instance pipeline
    VkPipelineShaderStageCreateInfo instance_vert_shader_stage_info = instance_vert_shader.get_shader_stage_create_info();
    VkPipelineShaderStageCreateInfo instance_frag_shader_stage_info = instance_frag_shader.get_shader_stage_create_info();

    //Use instance vert and frag shaders
    shader_stages_info[0] = instance_vert_shader_stage_info;
    shader_stages_info[1] = instance_frag_shader_stage_info;

    //The instance pipeline uses all the input bindings and attribute descriptions
    vertex_input_state_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
    vertex_input_state_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());

    if (vkCreateGraphicsPipelines(vulkan_instance.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &instance_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    ///Per vertex pipeline
    VkPipelineShaderStageCreateInfo vert_shader_stage_info = vert_shader.get_shader_stage_create_info();
    VkPipelineShaderStageCreateInfo frag_shader_stage_info = frag_shader.get_shader_stage_create_info();

    //Use vertex vert and frag shaders
    shader_stages_info[0] = vert_shader_stage_info;
    shader_stages_info[1] = frag_shader_stage_info;

    //The vertex pipeline only uses the non-instanced input bindings and attribute descriptions
    //(we pass the same list but only look at the vertex specific ones)
    vertex_input_state_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_info.vertexAttributeDescriptionCount = 3;

    if (vkCreateGraphicsPipelines(vulkan_instance.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &vertex_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

/// <summary>
/// Creates the framebuffers that can be used as a draw target in the renderpass
/// </summary>
void Vulkan_Window::create_framebuffers()
{
    swap_chain.framebuffers.resize(swap_chain.image_views.size());

    for (size_t i = 0; i < swap_chain.image_views.size(); i++)
    {
        //The swap chain uses multiple images, the render pipeline uses a single depth buffer (protected by semaphores)
        std::array<VkImageView, 2> attachments = { swap_chain.image_views[i], depth_image.image_view };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = swap_chain.extent.width;
        framebuffer_info.height = swap_chain.extent.height;
        framebuffer_info.layers = 1; //Only single layer images in the swap chain

        if (vkCreateFramebuffer(vulkan_instance.device, &framebuffer_info, nullptr, &swap_chain.framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

/// <summary>
/// Creates the command pool that manages the memory that stores the buffers
/// </summary>
void Vulkan_Window::create_command_pool()
{
    Queue_Family_Indices queue_family_indices = vulkan_instance.get_queue_families(surface);

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.pNext = nullptr;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

    if (vkCreateCommandPool(vulkan_instance.device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void Vulkan_Window::create_depth_resources()
{
    VkFormat depth_format = vulkan_instance.find_depth_format();

    depth_image.create_image(
        &vulkan_instance,
        swap_chain.extent.width, swap_chain.extent.height,
        depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    depth_image.create_image_view();
}

/// <summary>
/// Creates the memory buffer containing our vertex data
/// </summary>
void Vulkan_Window::create_vertex_buffer()
{
    VkDeviceSize buffer_size = konata_model.get_vertices_size();

    //Create staging buffer that transfers data between the host and device
    Buffer staging_buffer;
    staging_buffer.create(vulkan_instance, buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(vulkan_instance.device, staging_buffer.device_memory, 0, buffer_size, 0, &data);
    memcpy(data, konata_model.get_vertices_ptr(), (size_t)buffer_size);
    vkUnmapMemory(vulkan_instance.device, staging_buffer.device_memory);

    //Create vertex buffer as device only buffer
    vertex_buffer.create(vulkan_instance, buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //Copy data from host to device
    copy_buffer(staging_buffer.buffer, vertex_buffer.buffer, buffer_size);

    //Data on device, cleanup temp buffers
    staging_buffer.destroy(vulkan_instance.device);
}

void Vulkan_Window::create_index_buffer()
{
    VkDeviceSize buffer_size = konata_model.get_indices_size();
    Buffer staging_buffer;
    staging_buffer.create(vulkan_instance, buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(vulkan_instance.device, staging_buffer.device_memory, 0, buffer_size, 0, &data);
    memcpy(data, konata_model.get_indices_ptr(), (size_t)buffer_size);
    vkUnmapMemory(vulkan_instance.device, staging_buffer.device_memory);

    //Create index buffer as device only buffer
    index_buffer.create(vulkan_instance, buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //Copy data from host to device
    copy_buffer(staging_buffer.buffer, index_buffer.buffer, buffer_size);

    //Data on device, cleanup temp buffers
    staging_buffer.destroy(vulkan_instance.device);
}

void Vulkan_Window::create_instance_buffers()
{
    {
        ///Instance vertex buffer
        VkDeviceSize vertex_buffer_size = instance_konata.model.get_vertices_size();

        //Create staging buffer that transfers data between the host and device
        Buffer staging_buffer;
        staging_buffer.create(vulkan_instance, vertex_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(vulkan_instance.device, staging_buffer.device_memory, 0, vertex_buffer_size, 0, &data);
        memcpy(data, instance_konata.model.get_vertices_ptr(), (size_t)vertex_buffer_size);
        vkUnmapMemory(vulkan_instance.device, staging_buffer.device_memory);

        //Create vertex buffer as device only buffer
        instance_vertex_buffer.create(vulkan_instance, vertex_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        //Copy data from host to device
        copy_buffer(staging_buffer.buffer, instance_vertex_buffer.buffer, vertex_buffer_size);

        //Data on device, cleanup temp buffers
        staging_buffer.destroy(vulkan_instance.device);
    }
    {
        ///Instance index buffer
        VkDeviceSize index_buffer_size = instance_konata.model.get_indices_size();
        Buffer staging_buffer;
        staging_buffer.create(vulkan_instance, index_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(vulkan_instance.device, staging_buffer.device_memory, 0, index_buffer_size, 0, &data);
        memcpy(data, instance_konata.model.get_indices_ptr(), (size_t)index_buffer_size);
        vkUnmapMemory(vulkan_instance.device, staging_buffer.device_memory);

        //Create index buffer as device only buffer
        instance_index_buffer.create(vulkan_instance, index_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        //Copy data from host to device
        copy_buffer(staging_buffer.buffer, instance_index_buffer.buffer, index_buffer_size);

        //Data on device, cleanup temp buffers
        staging_buffer.destroy(vulkan_instance.device);
    }

    {
        ///Instance data buffer
        const int instance_count = 10;
        //konata_instances_data.resize(instance_count);

        float pos_x = 0.0f;
        float pos_y = 0.0f;

        for (size_t i = 0; i < instance_count; i++)
        {
            Instance_Data instance_data;
            instance_data.position = glm::vec3(pos_x, pos_y, 1.0f);
            instance_data.rotation = glm::vec3(0, 0, 0);
            instance_data.scale = 1.0f;
            instance_data.texture_index = 0;

            konata_instances_data.push_back(instance_data);

            if (pos_x >= 30.f)
            {
                pos_x = 0.0f;
                pos_y += 10.0f;
            }
            else
            {
                pos_x += 10.f;
            }
        }

        VkDeviceSize instance_data_buffer_size = sizeof(Instance_Data) * konata_instances_data.size();
        Buffer staging_buffer;
        staging_buffer.create(vulkan_instance, instance_data_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(vulkan_instance.device, staging_buffer.device_memory, 0, instance_data_buffer_size, 0, &data);
        memcpy(data, instance_konata.model.get_indices_ptr(), (size_t)instance_data_buffer_size);
        vkUnmapMemory(vulkan_instance.device, staging_buffer.device_memory);

        //Create vertex buffer as device only buffer
        instance_data_buffer.create(vulkan_instance, instance_data_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        //Copy data from host to device
        copy_buffer(staging_buffer.buffer, instance_data_buffer.buffer, instance_data_buffer_size);

        //Data on device, cleanup temp buffers
        staging_buffer.destroy(vulkan_instance.device);
    }
}

void Vulkan_Window::create_uniform_buffers()
{
    VkDeviceSize buffer_size = sizeof(MVP);

    uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        uniform_buffers[i].create(vulkan_instance, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        //Map pointer to memory so we can write to the buffer, this pointer is persistant throughout the applications lifetime
        vkMapMemory(vulkan_instance.device, uniform_buffers[i].device_memory, 0, buffer_size, 0, &uniform_buffers_mapped[i]);
    }
}

void Vulkan_Window::create_descriptor_pool()
{
    std::array<VkDescriptorPoolSize, 2> pool_sizes{};

    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2; // Two for each descriptor set in Descriptor_Sets
    pool_info.flags = 0;

    if (vkCreateDescriptorPool(vulkan_instance.device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void Vulkan_Window::create_descriptor_sets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);

    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocate_info.pSetLayouts = layouts.data(); //Uniform layout

    ///Triangle descriptor sets
    descriptor_sets.tri_descriptor_set.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(vulkan_instance.device, &allocate_info, descriptor_sets.tri_descriptor_set.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    //Describe the data layout inside the buffers we want to write
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniform_buffers[i].buffer;
        buffer_info.offset = 0;
        buffer_info.range = sizeof(MVP);

        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = texture_image.image_view;
        image_info.sampler = texture_image.sampler;

        //Tri shaders
        std::array<VkWriteDescriptorSet, 2> descriptor_writes{};

        //Descriptor for the MVP uniform
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = descriptor_sets.tri_descriptor_set[i]; //Binding to update
        descriptor_writes[0].dstBinding = 0; //Binding index equal to shader binding index
        descriptor_writes[0].dstArrayElement = 0; //Index of array data to update, no array so zero
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1; //A single buffer info struct
        descriptor_writes[0].pBufferInfo = &buffer_info; //Data and layout of buffer
        descriptor_writes[0].pImageInfo = nullptr; //Optional, only used for image data
        descriptor_writes[0].pTexelBufferView = nullptr; //Optional, only used for buffers views (for tex buffers)

        //Descriptor for the image sampler uniform
        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = descriptor_sets.tri_descriptor_set[i]; //Binding to update
        descriptor_writes[1].dstBinding = 1; //Binding index equal to shader binding index
        descriptor_writes[1].dstArrayElement = 0; //Index of array data to update, no array so zero
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[1].descriptorCount = 1; //A single buffer info struct
        descriptor_writes[1].pImageInfo = &image_info; //This is an image sampler so we provide an image data description

        //Apply updates
        //Only write descriptor, no copy, so copy variable is 0
        vkUpdateDescriptorSets(vulkan_instance.device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
    }

    ///Instance descriptor sets
    descriptor_sets.instance_descriptor_set.resize(MAX_FRAMES_IN_FLIGHT);

    if (VkResult result = vkAllocateDescriptorSets(vulkan_instance.device, &allocate_info, descriptor_sets.instance_descriptor_set.data()); result != VK_SUCCESS)
    {
        std::string error_string{ string_VkResult(result) };
        throw std::runtime_error("Failed to allocate descriptor sets! " + error_string);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniform_buffers[i].buffer;
        buffer_info.offset = 0;
        buffer_info.range = sizeof(MVP);

        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //image_info.imageView = instance_konata.texture.image_view;
        //image_info.sampler = instance_konata.texture.sampler;

        image_info.imageView = texture_image.image_view;
        image_info.sampler = texture_image.sampler;


        std::array<VkWriteDescriptorSet, 2> descriptor_writes{};

        //Descriptor for the MVP uniform
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = descriptor_sets.instance_descriptor_set[i]; //Binding to update
        descriptor_writes[0].dstBinding = 0; //Binding index equal to shader binding index
        descriptor_writes[0].dstArrayElement = 0; //Index of array data to update, no array so zero
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1; //A single buffer info struct
        descriptor_writes[0].pBufferInfo = &buffer_info; //Data and layout of buffer
        descriptor_writes[0].pImageInfo = nullptr; //Optional, only used for image data
        descriptor_writes[0].pTexelBufferView = nullptr; //Optional, only used for buffers views (for tex buffers)

        //Descriptor for the image sampler uniform
        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = descriptor_sets.instance_descriptor_set[i]; //Binding to update
        descriptor_writes[1].dstBinding = 1; //Binding index equal to shader binding index
        descriptor_writes[1].dstArrayElement = 0; //Index of array data to update, no array so zero
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[1].descriptorCount = 1; //A single buffer info struct
        descriptor_writes[1].pImageInfo = &image_info; //This is an image sampler so we provide an image data description

        //Apply updates
        vkUpdateDescriptorSets(vulkan_instance.device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
    }

}

void Vulkan_Window::create_command_buffer()
{
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //Directly submits to command queue (secondary can be used by primary)
    alloc_info.commandBufferCount = (uint32_t)command_buffers.size(); //Double buffer for concurrent frame processing

    if (vkAllocateCommandBuffers(vulkan_instance.device, &alloc_info, command_buffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void Vulkan_Window::create_sync_objects()
{
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Start in signaled state for first frame

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(vulkan_instance.device, &semaphore_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vulkan_instance.device, &semaphore_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vulkan_instance.device, &fence_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create semaphores!");
        }
    }
}

void Vulkan_Window::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only used by graphics queue
    buffer_info.flags = 0;

    if (vkCreateBuffer(vulkan_instance.device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    //Query the physical device requirements to use the vertex buffer
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vulkan_instance.device, buffer, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    //The memory has to be writable from the CPU side
    //Also, ensure the mapped memory matches the content of the allocated memory (no flush needed)
    allocate_info.memoryTypeIndex = vulkan_instance.find_memory_type(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(vulkan_instance.device, &allocate_info, nullptr, &buffer_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    //Bind the buffer to the allocated memory
    vkBindBufferMemory(vulkan_instance.device, buffer, buffer_memory, 0);
}

/// <summary>
/// Creates a temporary command buffer that transfers data between scr and dst buffers, executed immediately
/// </summary>
/// <param name="src_buffer"></param>
/// <param name="dst_buffer"></param>
/// <param name="size"></param>
void Vulkan_Window::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    //Record copy
    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0; // Optional
    copy_region.dstOffset = 0; // Optional
    copy_region.size = size;

    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    end_single_time_commands(command_buffer);
}

void Vulkan_Window::create_texture_image()
{
    int texture_width;
    int texture_height;
    int texture_channels;

    //Load texture image, force alpha channel
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
    VkDeviceSize image_size = texture_width * texture_height * 4;

    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image!");
    }

    //Setup host visible staging buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

    //Copy the texture data into the staging buffer
    void* data;
    vkMapMemory(vulkan_instance.device, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(vulkan_instance.device, staging_buffer_memory);

    stbi_image_free(pixels);

    //Texels == pixels

    texture_image.create_image(&vulkan_instance, texture_width, texture_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    //Change layout of target image memory to be optimal for writing destination
    {
        VkCommandBuffer command_buffer = begin_single_time_commands();

        texture_image.transition_image_layout(command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        end_single_time_commands(command_buffer);
    }

    //Transfer the image data from the staging buffer to the image memory
    copy_buffer_to_image(staging_buffer, texture_image.image, texture_image.width, texture_image.height);

    //Change layout of image memory to be optimal for reading by a shader
    {
        VkCommandBuffer command_buffer = begin_single_time_commands();

        texture_image.transition_image_layout(command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        end_single_time_commands(command_buffer);
    }

    vkDestroyBuffer(vulkan_instance.device, staging_buffer, nullptr);
    vkFreeMemory(vulkan_instance.device, staging_buffer_memory, nullptr);
}

void Vulkan_Window::transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
    //Todo move to command class?
    VkCommandBuffer command_buffer = begin_single_time_commands();

    end_single_time_commands(command_buffer);
}

void Vulkan_Window::create_texture_image_view()
{
    //texture_image_view = create_image_view(vulkan_instance.device, texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    texture_image.create_image_view();
}

void Vulkan_Window::create_texture_sampler()
{
    texture_image.create_texture_sampler();
}

void Vulkan_Window::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0; //Start of pixel values
    region.bufferRowLength = 0; //Memory layout
    region.bufferImageHeight = 0; //Memory layout

    //Part of the image we want to copy
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0,0,0 };
    region.imageExtent = { width, height, 1 };

    //Enqueue the image copy operation
    vkCmdCopyBufferToImage(
        command_buffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    end_single_time_commands(command_buffer);
}

void Vulkan_Window::record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index)
{

    //Describe a new render pass targeting the given image index in the swapchain
    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

    //attach this render pass to the swap chain image
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.framebuffer = swap_chain.framebuffers[image_index];

    //Cover the whole swap chain image
    render_pass_begin_info.renderArea.offset = { 0,0 };
    render_pass_begin_info.renderArea.extent = swap_chain.extent;

    //Clear to black (we use VK_ATTACHMENT_LOAD_OP_CLEAR)
    //Clear order should be same as attachment order
    std::array<VkClearValue, 2> clear_colors{};
    clear_colors[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } }; //Clear image to black
    clear_colors[1].depthStencil = { 1.0f, 0 }; //Clear depth image to 1.0 (far plane)
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_colors.size());
    render_pass_begin_info.pClearValues = clear_colors.data();

    //We set viewport and scissor to dynamic earlier, so we define them now
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_chain.extent.width);
    viewport.height = static_cast<float>(swap_chain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = swap_chain.extent;

    //Start recording a command buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }


    //VK_SUBPASS_CONTENTS_INLINE means we don't use secondary command buffers
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);


    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    //Render the object

    //Set the vertex buffers
    std::array<VkBuffer, 1>  vertex_buffers = { vertex_buffer.buffer };
    std::array<VkDeviceSize, 1> offsets = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, vertex_buffers.size(), vertex_buffers.data(), offsets.data());

    //Set the index buffers
    vkCmdBindIndexBuffer(command_buffer, index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    //Set the uniform buffers
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.tri_descriptor_set[current_frame], 0, nullptr);

    //Bind to graphics pipeline: The shaders and configuration used to the render the object
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vertex_pipeline);

    //Draw command, set vertex and instance counts (we're not using instancing) and indices
    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(konata_model.indices.size()), 1, 0, 0, 0);

    ////Instanced Konatas
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.instance_descriptor_set[current_frame], 0, nullptr);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, instance_pipeline);
    //Binding point 0 - mesh vertex buffer
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &instance_vertex_buffer.buffer, offsets.data());
    //Binding point 1 - instance data buffer
    vkCmdBindVertexBuffers(command_buffer, 1, 1, &instance_data_buffer.buffer, offsets.data());
    //Bind index buffer
    vkCmdBindIndexBuffer(command_buffer, instance_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    //Render instances
    int instance_count = 10;
    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(instance_konata.model.indices.size()), instance_count, 0, 0, 0);
    ////

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer!");
    }
}


void Vulkan_Window::cleanup_swap_chain()
{
    //Destroy objects that depend on the swap chain
    depth_image.destroy();

    //Destroy the swap chain
    swap_chain.cleanup_swap_chain();
}

void Vulkan_Window::recreate_swap_chain()
{
    int new_width = 0;
    int new_height = 0;
    glfwGetFramebufferSize(window, &new_width, &new_height);

    while (new_width == 0 || new_height == 0) {
        glfwGetFramebufferSize(window, &new_width, &new_height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(vulkan_instance.device);

    cleanup_swap_chain();

    swap_chain.create_swap_chain(window, surface);

    create_depth_resources(); //Depend on depth image
    create_framebuffers(); //Depend on image views
}

/// <summary>
/// Begins a single-use command buffer for short-lived operations.
/// Allocates a primary command buffer from the specified command pool, 
/// begins recording with the VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT flag, 
/// and returns the command buffer.
/// </summary>
/// <returns>
/// The allocated and ready-to-record VkCommandBuffer.
/// </returns>
VkCommandBuffer Vulkan_Window::begin_single_time_commands()
{

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(vulkan_instance.device, &allocate_info, &command_buffer);

    //Start recording command buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //We only use this buffer once

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

void Vulkan_Window::end_single_time_commands(VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    //Execute copy command buffer instantly and wait until transfer is complete
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    vkQueueSubmit(vulkan_instance.graphics_queue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(vulkan_instance.graphics_queue);

    vkFreeCommandBuffers(vulkan_instance.device, command_pool, 1, &command_buffer);
}

VkShaderModule Vulkan_Window::create_shader_module(const std::vector<char>& bytecode)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = bytecode.size();
    create_info.pCode = reinterpret_cast<const uint32_t*> (bytecode.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(vulkan_instance.device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shader_module;
}

bool Vulkan_Window::has_stencil_component(VkFormat format) const
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}