#include "pch.h"
#include "vulkan_engine.h"

namespace vulvox
{
    Vulkan_Engine::Vulkan_Engine() : swap_chain(&vulkan_instance)
    {
    }

    Vulkan_Engine::~Vulkan_Engine()
    {
    }

    void Vulkan_Engine::init_window(uint32_t width, uint32_t height)
    {
        this->width = width;
        this->height = height;

        std::cout << "Init window.." << std::endl;

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

        //Pass pointer of this to glfw so we can retrieve the object instance in the callback function
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

        std::cout << "Window initialized." << std::endl;
    }

    void Vulkan_Engine::init_vulkan()
    {
        std::cout << "Init vulkan.." << std::endl;

        vulkan_instance.init_instance();
        vulkan_instance.init_surface(window);
        vulkan_instance.init_device();
        vulkan_instance.init_allocator();

        swap_chain.create_swap_chain(window, vulkan_instance.surface);
        mvp_handler.set_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));

        create_render_pass();
        create_mvp_descriptor_set_layout();
        create_texture_descriptor_set_layout();
        create_graphics_pipeline();
        command_pool = Vulkan_Command_Pool(&vulkan_instance, MAX_FRAMES_IN_FLIGHT);
        create_depth_resources();
        create_framebuffers();

        create_uniform_buffers();
        create_instance_buffers();
        create_instance_texture_buffers();
        create_descriptor_pool();
        create_descriptor_sets();
        create_sync_objects();

        std::cout << "Vulkan initialized." << std::endl;
    }

    void Vulkan_Engine::init(uint32_t width, uint32_t height)
    {
        init_window(width, height);
        init_vulkan();
        is_initialized = true;
    }

    void Vulkan_Engine::destroy()
    {
        if (!is_initialized)
        {
            return;
        }

        //Wait until all operations are completed before cleanup
        vkDeviceWaitIdle(vulkan_instance.device);

        cleanup_swap_chain();

        vkDestroyPipeline(vulkan_instance.device, vertex_pipeline, nullptr);
        vkDestroyPipeline(vulkan_instance.device, instance_pipeline, nullptr);
        vkDestroyPipeline(vulkan_instance.device, instance_tex_array_pipeline, nullptr);

        vkDestroyPipelineLayout(vulkan_instance.device, pipeline_layout, nullptr);
        vkDestroyRenderPass(vulkan_instance.device, render_pass, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            uniform_buffers[i].destroy(vulkan_instance.allocator);
        }

        //Descriptor sets will be destroyed with the pool
        vkDestroyDescriptorPool(vulkan_instance.device, descriptor_pool, nullptr);


        //Texture cleanup
        for (auto& [name, texture] : textures)
        {
            texture.destroy();
        }

        textures.clear();

        for (auto& [name, texture_array] : texture_arrays)
        {
            texture_array.destroy();
        }

        texture_arrays.clear();


        //Cleanup descriptor set layout and buffers
        vkDestroyDescriptorSetLayout(vulkan_instance.device, mvp_descriptor_set_layout, nullptr);
        vkDestroyDescriptorSetLayout(vulkan_instance.device, texture_descriptor_set_layout, nullptr);

        //Clear all the models and their (vertex & index) buffers
        for (auto& [name, model] : models)
        {
            model.destroy();
        }

        models.clear();

        //Destroy instance buffers
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            instance_data_buffers[i].destroy(vulkan_instance.allocator);
            instance_texture_index_buffers[i].destroy(vulkan_instance.allocator);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(vulkan_instance.device, image_available_semaphores.at(i), nullptr);
            vkDestroySemaphore(vulkan_instance.device, render_finished_semaphores.at(i), nullptr);
            vkDestroyFence(vulkan_instance.device, in_flight_fences.at(i), nullptr);
        }

        command_pool.destroy();

        vulkan_instance.cleanup_allocator();
        vulkan_instance.cleanup_device();
        vulkan_instance.cleanup_surface();
        vulkan_instance.cleanup_instance();

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    GLFWwindow* Vulkan_Engine::get_glfw_window_ptr()
    {
        return window;
    }

    MVP_Handler& Vulkan_Engine::get_mvp_handler()
    {
        return mvp_handler;
    }

    void Vulkan_Engine::resize_window(const uint32_t new_width, const uint32_t new_height)
    {
        this->width = new_width;
        this->height = new_height;

        //Will also call the resize callback so the render engine will recreate the swapchain
        glfwSetWindowSize(get_glfw_window_ptr(), new_width, new_height);
    }

    void Vulkan_Engine::load_model(const std::string& model_name, const std::filesystem::path& path)
    {
        if (models.contains(model_name))
        {
            std::cout << "Attempted to load model " << model_name << " but a model with the same name was already loaded. Path was: " << path << std::endl;
            return;
        }

        auto [model_it, succeeded] = models.try_emplace(model_name, &vulkan_instance, command_pool, path);

        if (!succeeded)
        {
            std::string error_string = "Failed to load model " + model_name + " with path: " + path.string();
            throw std::runtime_error(error_string);
        }
    }

    void Vulkan_Engine::load_texture(const std::string& texture_name, const std::filesystem::path& path)
    {
        if (textures.contains(texture_name))
        {
            std::cout << "Attempted to load texture " << texture_name << " but a texture with the same name was already loaded. Path was: " << path << std::endl;
            return;
        }

        auto [texture_it, succeeded] = textures.try_emplace(texture_name, Image::create_texture_image(vulkan_instance, command_pool, path));

        if (!succeeded)
        {
            std::string error_string = "Failed to load texture " + texture_name + " with path: " + path.string();
            throw std::runtime_error(error_string);
        }


        auto [texture_descriptor_it, descriptor_succeeded] = texture_descriptor_sets.try_emplace(texture_name, create_texture_descriptor_set(texture_it->second));

        if (!descriptor_succeeded)
        {
            throw std::runtime_error("Failed to allocate descriptor sets! (map allocation failed)");
        }
    }

    void Vulkan_Engine::load_texture_array(const std::string& texture_name, const std::vector<std::filesystem::path>& paths)
    {
        if (texture_arrays.contains(texture_name))
        {
            std::cout << "Attempted to load texture array " << texture_name << " but a texture array with the same name was already loaded." << std::endl;
            return;
        }

        auto [texture_it, succeeded] = texture_arrays.try_emplace(texture_name, Image::create_texture_array_image(vulkan_instance, command_pool, paths));

        if (!succeeded)
        {
            std::string error_string = "Failed to load texture array " + texture_name;
            throw std::runtime_error(error_string);
        }

        auto [texture_descriptor_it, descriptor_succeeded] = texture_array_descriptor_sets.try_emplace(texture_name, create_texture_descriptor_set(texture_it->second));

        if (!descriptor_succeeded)
        {
            throw std::runtime_error("Failed to allocate descriptor sets! (map allocation failed)");
        }

    }

    void Vulkan_Engine::unload_model(const std::string& name)
    {
    }

    void Vulkan_Engine::unload_texture(const std::string& name)
    {
    }

    void Vulkan_Engine::unload_texture_array(const std::string& name)
    {
    }

    void Vulkan_Engine::start_draw()
    {
        vkWaitForFences(vulkan_instance.device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

        //Ask the swapchain for a render image to target
        VkResult result = vkAcquireNextImageKHR(vulkan_instance.device, swap_chain.swap_chain, UINT64_MAX, image_available_semaphores[current_frame], nullptr, &current_image_index);

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

        //Reset fence *after* confirming the swapchain is valid (prevents deadlock)
        vkResetFences(vulkan_instance.device, 1, &in_flight_fences[current_frame]);

        //Update global variables (camera etc.)
        update_uniform_buffer(current_frame);

        //Start recording a new command buffer for rendering
        current_command_buffer = command_pool.reset_command_buffer(current_frame);

        // Start recording the command buffer and wait for draw calls
        start_record_command_buffer();
    }

    void Vulkan_Engine::end_draw()
    {
        //Complete the command buffer before submitting it and presenting the image
        end_record_command_buffer();

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        //Which semaphore to wait for before execution and at which stage
        std::array<VkSemaphore, 1> wait_semaphores = { image_available_semaphores[current_frame] };
        std::array<VkPipelineStageFlags, 1> wait_stages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();

        //Which semaphore to signal when the command buffer is done
        std::array<VkSemaphore, 1> signal_semaphores = { render_finished_semaphores[current_frame] };
        submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
        submit_info.pSignalSemaphores = signal_semaphores.data();

        //Link command buffer
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &current_command_buffer;

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
        present_info.swapchainCount = static_cast<uint32_t>(swap_chains.size());
        present_info.pSwapchains = swap_chains.data();
        present_info.pImageIndices = &current_image_index;

        //Dont need to collect the draw results because the present function returns them as well
        present_info.pResults = nullptr;

        //Present the result on screen
        VkResult result = vkQueuePresentKHR(vulkan_instance.present_queue, &present_info);

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

    void Vulkan_Engine::draw_model(const std::string& model_name, const std::string& texture_name, const glm::mat4& model_matrix)
    {
        if (!models.contains(model_name))
        {
            std::cout << "No model with name " << model_name << " is loaded, skipping draw call." << std::endl;
            return;
        }

        if (!textures.contains(texture_name))
        {
            std::cout << "No texture with name " << texture_name << " is loaded, skipping draw call." << std::endl;
            return;
        }

        //Set the vertex buffers
        std::array<VkDeviceSize, 1> offsets = { 0 };
        vkCmdBindVertexBuffers(current_command_buffer, 0, 1, &models.at(model_name).vertex_buffer.buffer, offsets.data());

        //Set the index buffers
        std::vector<VkBuffer> index_buffers;
        vkCmdBindIndexBuffer(current_command_buffer, models.at(model_name).index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        //Bind the uniform buffers
        //Bind set 0, the MVP buffer
        vkCmdBindDescriptorSets(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.tri_descriptor_set[current_frame], 0, nullptr);
        //Bind set 1, the texture
        vkCmdBindDescriptorSets(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &texture_descriptor_sets.at(texture_name), 0, nullptr);

        //Set the push constants (model matrix)
        vkCmdPushConstants(current_command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model_matrix);

        //Bind to graphics pipeline: The shaders and configuration used to the render the object
        vkCmdBindPipeline(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vertex_pipeline);

        //Draw command, set vertex and instance counts (we're not using instancing here) and indices
        vkCmdDrawIndexed(current_command_buffer, models.at(model_name).index_count, 1, 0, 0, 0);
    }

    void Vulkan_Engine::draw_model_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const int texture_index, const glm::mat4& model_matrix)
    {
        if (false)
        {
            std::cout << "draw_model_with_texture_array is not yet supported." << std::endl;
            return;
        }

        if (!models.contains(model_name))
        {
            std::cout << "No model with name " << model_name << " is loaded, skipping draw call." << std::endl;
            return;
        }

        if (!texture_arrays.contains(texture_array_name))
        {
            std::cout << "No texture array with name " << texture_array_name << " is loaded, skipping draw call." << std::endl;
            return;
        }

        std::array<VkDeviceSize, 1> offsets = { 0 };

        //Bind the uniform buffers
        //Bind set 0, the MVP buffer
        vkCmdBindDescriptorSets(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.tri_descriptor_set[current_frame], 0, nullptr);
        //Bind set 1, the texture
        vkCmdBindDescriptorSets(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &texture_array_descriptor_sets.at(texture_array_name), 0, nullptr);

        //Binding point 0 - mesh vertex buffer
        vkCmdBindVertexBuffers(current_command_buffer, 0, 1, &models.at(model_name).vertex_buffer.buffer, offsets.data());

        ////Binding point 1 - instance data buffer
        //vkCmdBindVertexBuffers(current_command_buffer, 1, 1, &instance_data_buffers[current_frame].buffer, offsets.data());

        ////Binding point 2 - texture array index buffer
        //vkCmdBindVertexBuffers(current_command_buffer, 2, 1, &instance_texture_index_buffers[current_frame].buffer, offsets.data());

        //Set the push constants (model matrix)
        vkCmdPushConstants(current_command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model_matrix);

        //Bind index buffer
        vkCmdBindIndexBuffer(current_command_buffer, models.at(model_name).index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindPipeline(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vertex_pipeline);

        //Draw command, set vertex and instance counts (we're not using instancing here) and indices
        vkCmdDrawIndexed(current_command_buffer, models.at(model_name).index_count, 1, 0, 0, 0);
    }

    void Vulkan_Engine::draw_instanced(const std::string& model_name, const std::string& texture_name, const std::vector<glm::mat4>& model_matrices)
    {
        if (!models.contains(model_name))
        {
            std::cout << "No model with name " << model_name << " is loaded, skipping draw call." << std::endl;
            return;
        }

        if (!textures.contains(texture_name))
        {
            std::cout << "No texture with name " << texture_name << " is loaded, skipping draw call." << std::endl;
            return;
        }

        std::array<VkDeviceSize, 1> offsets = { 0 };

        copy_to_instance_buffer(model_matrices);

        //Bind the uniform buffers
        //Bind set 0, the MVP buffer
        vkCmdBindDescriptorSets(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.instance_descriptor_set[current_frame], 0, nullptr);
        //Bind set 1, the texture
        vkCmdBindDescriptorSets(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &texture_descriptor_sets.at(texture_name), 0, nullptr);

        //Binding point 0 - mesh vertex buffer
        vkCmdBindVertexBuffers(current_command_buffer, 0, 1, &models.at(model_name).vertex_buffer.buffer, offsets.data());

        //Binding point 1 - instance data buffer
        vkCmdBindVertexBuffers(current_command_buffer, 1, 1, &instance_data_buffers[current_frame].buffer, offsets.data());

        //Bind index buffer
        vkCmdBindIndexBuffer(current_command_buffer, models.at(model_name).index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindPipeline(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, instance_pipeline);

        //Render instances
        uint32_t instance_count = static_cast<uint32_t>(model_matrices.size());
        vkCmdDrawIndexed(current_command_buffer, models.at(model_name).index_count, instance_count, 0, 0, 0);
    }

    void Vulkan_Engine::draw_instanced_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const std::vector<glm::mat4>& model_matrices, const std::vector<uint32_t>& texture_indices)
    {
        if (!models.contains(model_name))
        {
            std::cout << "No model with name " << model_name << " is loaded, skipping draw call." << std::endl;
            return;
        }

        if (!texture_arrays.contains(texture_array_name))
        {
            std::cout << "No texture array with name " << texture_array_name << " is loaded, skipping draw call." << std::endl;
            return;
        }

        std::array<VkDeviceSize, 1> offsets = { 0 };

        copy_to_instance_buffer(model_matrices);
        copy_to_instance_texture_buffer(texture_indices);

        //Bind the uniform buffers
        //Bind set 0, the MVP buffer
        vkCmdBindDescriptorSets(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.instance_descriptor_set[current_frame], 0, nullptr);
        //Bind set 1, the texture
        vkCmdBindDescriptorSets(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &texture_array_descriptor_sets.at(texture_array_name), 0, nullptr);

        //Binding point 0 - mesh vertex buffer
        vkCmdBindVertexBuffers(current_command_buffer, 0, 1, &models.at(model_name).vertex_buffer.buffer, offsets.data());

        //Binding point 1 - instance data buffer
        vkCmdBindVertexBuffers(current_command_buffer, 1, 1, &instance_data_buffers[current_frame].buffer, offsets.data());

        //Binding point 2 - texture array index buffer
        vkCmdBindVertexBuffers(current_command_buffer, 2, 1, &instance_texture_index_buffers[current_frame].buffer, offsets.data());

        //Bind index buffer
        vkCmdBindIndexBuffer(current_command_buffer, models.at(model_name).index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindPipeline(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, instance_tex_array_pipeline);

        //Render instances
        uint32_t instance_count = static_cast<uint32_t>(model_matrices.size());
        vkCmdDrawIndexed(current_command_buffer, models.at(model_name).index_count, instance_count, 0, 0, 0);
    }

    void Vulkan_Engine::update_uniform_buffer(uint32_t current_image)
    {
        memcpy(uniform_buffers[current_image].allocation_info.pMappedData, &mvp_handler.model_view_projection, sizeof(mvp_handler.model_view_projection));
    }

    void Vulkan_Engine::recreate_swap_chain()
    {
        int new_width = 0;
        int new_height = 0;
        glfwGetFramebufferSize(window, &new_width, &new_height);

        while (new_width == 0 || new_height == 0) {
            glfwGetFramebufferSize(window, &new_width, &new_height);
            glfwWaitEvents();
        }

        width = new_width;
        height = new_height;

        vkDeviceWaitIdle(vulkan_instance.device);

        cleanup_swap_chain();

        swap_chain.create_swap_chain(window, vulkan_instance.surface);

        mvp_handler.set_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));

        create_depth_resources(); //Depend on depth image
        create_framebuffers(); //Depend on image views
    }

    void Vulkan_Engine::cleanup_swap_chain()
    {
        //Destroy objects that depend on the swap chain
        depth_image.destroy();

        //Destroy the swap chain
        swap_chain.cleanup_swap_chain();
    }

    void Vulkan_Engine::create_render_pass()
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

    void Vulkan_Engine::create_graphics_pipeline()
    {
        //Define push constants, the model matrix for single rendering is updated using push constants
        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(glm::mat4);

        //Define global variables (like a MVP matrix)
        //These are defined in a seperate pipeline layout
        std::array<VkDescriptorSetLayout, 2> set_layouts = { mvp_descriptor_set_layout, texture_descriptor_set_layout };

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(set_layouts.size()); //Amount of descriptor set layouts (uniform buffer layouts)
        pipeline_layout_info.pSetLayouts = set_layouts.data();
        pipeline_layout_info.pushConstantRangeCount = 1; //Small uniform data, used for the model matrix
        pipeline_layout_info.pPushConstantRanges = &push_constant_range;

        if (vkCreatePipelineLayout(vulkan_instance.device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        //Load compiled SPIR-V shader files 
        std::filesystem::path vert_shader_filepath("../shaders/vert.spv");
        std::filesystem::path frag_shader_filepath("../shaders/frag.spv");

        std::filesystem::path instance_vert_shader_filepath("../shaders/instance_vert.spv");
        std::filesystem::path instance_frag_shader_filepath("../shaders/instance_frag.spv");

        std::filesystem::path instance_tex_array_vert_shader_filepath("../shaders/instance_tex_array_vert.spv");
        std::filesystem::path instance_tex_array_frag_shader_filepath("../shaders/instance_tex_array_frag.spv");

        Vulkan_Shader vert_shader{ vulkan_instance.device, vert_shader_filepath, "main", VK_SHADER_STAGE_VERTEX_BIT };
        Vulkan_Shader frag_shader{ vulkan_instance.device, frag_shader_filepath, "main", VK_SHADER_STAGE_FRAGMENT_BIT };
        Vulkan_Shader instance_vert_shader{ vulkan_instance.device, instance_vert_shader_filepath, "main", VK_SHADER_STAGE_VERTEX_BIT };
        Vulkan_Shader instance_frag_shader{ vulkan_instance.device, instance_frag_shader_filepath, "main", VK_SHADER_STAGE_FRAGMENT_BIT };
        Vulkan_Shader instance_vert_tex_array_shader{ vulkan_instance.device, instance_tex_array_vert_shader_filepath, "main", VK_SHADER_STAGE_VERTEX_BIT };
        Vulkan_Shader instance_frag_tex_array_shader{ vulkan_instance.device, instance_tex_array_frag_shader_filepath, "main", VK_SHADER_STAGE_FRAGMENT_BIT };

        //Describes the configuration of the vertices the triangles and lines use
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

        VkPipelineDynamicStateCreateInfo dynamic_state_info{};
        dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state_info.pDynamicStates = dynamic_states.data();

        VkPipelineViewportStateCreateInfo viewport_state_info{};
        viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_info.viewportCount = 1;
        viewport_state_info.scissorCount = 1;
        viewport_state_info.flags = 0;

        //Setup rasterizer stage
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
            Instance_Data::get_binding_description(1),
            //Binding point 2: Texture array index
            Texture_Array_Index_Binding::get_binding_description(2)
        };

        //Vertex attribute bindings
        //Note that the shader declaration for per-vertex and per-instance attributes is the same, the different input rates are only stored in the bindings:
            //	layout (location = 0) in vec3 in_position;		Per-Vertex
            //	...
            //	layout (location = 3) in vec3 instance_position;	Per-Instance
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

        //Per-vertex attributes
        //These are advanced for each vertex fetched by the vertex shader
        for (const auto& attribute_desc : Vertex::get_attribute_descriptions(0))
        {
            attribute_descriptions.push_back(attribute_desc);
        }

        //Per-Instance attributes
        //These are advanced for each instance rendered
        for (const auto& attribute_desc : Instance_Data::get_attribute_descriptions(1))
        {
            attribute_descriptions.push_back(attribute_desc);
        }

        attribute_descriptions.push_back(Texture_Array_Index_Binding::get_attribute_description(2));

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

        //The instance pipeline uses the input bindings and attribute descriptions except for the texture array index
        vertex_input_state_info.vertexBindingDescriptionCount = 2;
        vertex_input_state_info.vertexAttributeDescriptionCount = 7;

        if (vkCreateGraphicsPipelines(vulkan_instance.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &instance_pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        //Per instance pipeline with texture array support
        VkPipelineShaderStageCreateInfo instance_texture_array_vert_shader_stage_info = instance_vert_tex_array_shader.get_shader_stage_create_info();
        VkPipelineShaderStageCreateInfo instance_texture_array_frag_shader_stage_info = instance_frag_tex_array_shader.get_shader_stage_create_info();

        //Use instance vert and frag shaders
        shader_stages_info[0] = instance_texture_array_vert_shader_stage_info;
        shader_stages_info[1] = instance_texture_array_frag_shader_stage_info;

        //The instance pipeline uses all the input bindings and attribute descriptions
        vertex_input_state_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
        vertex_input_state_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());

        if (vkCreateGraphicsPipelines(vulkan_instance.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &instance_tex_array_pipeline) != VK_SUCCESS)
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
    /// e.g. depth and swap chain images
    /// </summary>
    void Vulkan_Engine::create_framebuffers()
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

    void Vulkan_Engine::create_depth_resources()
    {
        VkFormat depth_format = vulkan_instance.find_depth_format();

        depth_image.create_image(
            &vulkan_instance,
            swap_chain.extent.width, swap_chain.extent.height,
            depth_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            VMA_MEMORY_USAGE_AUTO);

        depth_image.create_image_view();
    }

    void Vulkan_Engine::create_instance_buffers()
    {
        const int base_instance_count = 50;

        VkDeviceSize instance_data_buffer_size = sizeof(glm::mat4) * base_instance_count;

        //Create instance buffers as device only buffers
        instance_data_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            instance_data_buffers[i].create(vulkan_instance, instance_data_buffer_size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        }
    }

    void Vulkan_Engine::create_instance_texture_buffers()
    {
        const int base_instance_count = 50;

        VkDeviceSize instance_texture_index_buffers_size = sizeof(uint32_t) * base_instance_count;

        //Create instance buffers as device only buffers
        instance_texture_index_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            instance_texture_index_buffers[i].create(vulkan_instance, instance_texture_index_buffers_size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        }
    }

    void Vulkan_Engine::copy_to_instance_buffer(const std::vector<glm::mat4>& model_matrices)
    {
        size_t data_size = model_matrices.size() * sizeof(model_matrices[0]);

        if (data_size > instance_data_buffers[current_frame].size)
        {
            instance_data_buffers[current_frame].recreate(vulkan_instance, data_size);
        }

        memcpy(instance_data_buffers[current_frame].allocation_info.pMappedData, model_matrices.data(), data_size);
    }

    void Vulkan_Engine::copy_to_instance_texture_buffer(const std::vector<uint32_t>& instance_texture_indices)
    {
        size_t data_size = instance_texture_indices.size() * sizeof(uint32_t);

        if (data_size > instance_texture_index_buffers[current_frame].size)
        {
            instance_texture_index_buffers[current_frame].recreate(vulkan_instance, data_size);
        }

        memcpy(instance_texture_index_buffers[current_frame].allocation_info.pMappedData, instance_texture_indices.data(), data_size);
    }

    void Vulkan_Engine::create_uniform_buffers()
    {
        VkDeviceSize buffer_size = sizeof(MVP);

        uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            //Also maps pointer to memory so we can write to the buffer, this pointer is persistant throughout the applications lifetime.
            uniform_buffers[i].create(vulkan_instance, buffer_size,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        }
    }

    /// <summary>
    /// This function creates a descriptor pool that holds the descriptor sets.
    /// Because this renderers usecase will not involve a lot of models we just create a single large pool.
    /// </summary>
    void Vulkan_Engine::create_descriptor_pool()
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
        pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) + 510; // Just allocate a bunch so we can load multiple models, can make dynamic later.
        pool_info.flags = 0;

        if (vkCreateDescriptorPool(vulkan_instance.device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    /// <summary>
    /// Creates the descriptor sets that describes the layout of the mvp matrix data binding in our pipeline.
    /// </summary>
    void Vulkan_Engine::create_mvp_descriptor_set_layout()
    {
        //Layout binding for the uniform buffer
        VkDescriptorSetLayoutBinding mvp_layout_binding{};
        mvp_layout_binding.binding = 0; //Same as in shader
        mvp_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        mvp_layout_binding.descriptorCount = 1;
        mvp_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //This buffer is (only) referenced in the vertex stage
        mvp_layout_binding.pImmutableSamplers = nullptr; //Optional

        //Bind the layout binding in the single descriptor set layout
        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = 1;
        layout_info.pBindings = &mvp_layout_binding;

        if (vkCreateDescriptorSetLayout(vulkan_instance.device, &layout_info, nullptr, &mvp_descriptor_set_layout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to created descriptor set layout!");
        }
    }

    /// <summary>
    /// Creates the descriptor sets that describes the layout of the texture binding in our pipeline.
    /// </summary>
    void Vulkan_Engine::create_texture_descriptor_set_layout()
    {
        //Layout binding for the image sampler
        VkDescriptorSetLayoutBinding sampler_layout_binding{};
        sampler_layout_binding.binding = 1; //Same as in shader
        sampler_layout_binding.descriptorCount = 1;
        sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_layout_binding.pImmutableSamplers = nullptr;
        sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //Connect to fragment shader stage

        //Bind the layout binding in the single descriptor set layout
        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = 1;
        layout_info.pBindings = &sampler_layout_binding;

        if (vkCreateDescriptorSetLayout(vulkan_instance.device, &layout_info, nullptr, &texture_descriptor_set_layout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to created descriptor set layout!");
        }
    }

    /// <summary>
    /// Allocates the mvp descriptor sets for all shaders.
    /// The MVP buffer will never change (only its data will) so we write it here as well.
    /// </summary>
    void Vulkan_Engine::create_descriptor_sets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mvp_descriptor_set_layout);

        VkDescriptorSetAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = descriptor_pool;
        allocate_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocate_info.pSetLayouts = layouts.data(); //mvp uniform layout

        ///Triangle descriptor sets
        descriptor_sets.tri_descriptor_set.resize(MAX_FRAMES_IN_FLIGHT);

        if (VkResult result = vkAllocateDescriptorSets(vulkan_instance.device, &allocate_info, descriptor_sets.tri_descriptor_set.data()); result != VK_SUCCESS)
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

            //Tri shaders
            VkWriteDescriptorSet descriptor_write{};

            //Descriptor for the MVP uniform
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = descriptor_sets.tri_descriptor_set[i]; //Binding to update
            descriptor_write.dstBinding = 0; //Binding index equal to shader binding index
            descriptor_write.dstArrayElement = 0; //Index of array data to update, no array so zero
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.descriptorCount = 1; //A single buffer info struct
            descriptor_write.pBufferInfo = &buffer_info; //Data and layout of buffer
            descriptor_write.pImageInfo = nullptr; //Optional, only used for image data
            descriptor_write.pTexelBufferView = nullptr; //Optional, only used for buffers views (for tex buffers)

            //Apply updates, Only write descriptor, no copy, so copy variable is 0
            vkUpdateDescriptorSets(vulkan_instance.device, 1, &descriptor_write, 0, nullptr);
        }

        ///Instance descriptor sets
        descriptor_sets.instance_descriptor_set.resize(MAX_FRAMES_IN_FLIGHT);

        if (VkResult result = vkAllocateDescriptorSets(vulkan_instance.device, &allocate_info, descriptor_sets.instance_descriptor_set.data()); result != VK_SUCCESS)
        {
            std::string error_string{ string_VkResult(result) };
            throw std::runtime_error("Failed to allocate descriptor sets! " + error_string);
        }

        //TODO: Do we need this or can we just use the above one for all shaders?
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo buffer_info{};
            buffer_info.buffer = uniform_buffers[i].buffer;
            buffer_info.offset = 0;
            buffer_info.range = sizeof(MVP);

            VkWriteDescriptorSet descriptor_write{};

            //Descriptor for the MVP uniform
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = descriptor_sets.instance_descriptor_set[i]; //Binding to update
            descriptor_write.dstBinding = 0; //Binding index equal to shader binding index
            descriptor_write.dstArrayElement = 0; //Index of array data to update, no array so zero
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.descriptorCount = 1; //A single buffer info struct
            descriptor_write.pBufferInfo = &buffer_info; //Data and layout of buffer
            descriptor_write.pImageInfo = nullptr; //Optional, only used for image data
            descriptor_write.pTexelBufferView = nullptr; //Optional, only used for buffers views (for tex buffers)

            //Apply updates
            vkUpdateDescriptorSets(vulkan_instance.device, 1, &descriptor_write, 0, nullptr);
        }
    }

    /// <summary>
    /// Allocates a new descriptor for the texture sampler uniform in the triangle shader to point to the given texture
    /// </summary>
    /// <param name="texture_name">Name of the texture to create a descriptor for</param>
    VkDescriptorSet Vulkan_Engine::create_texture_descriptor_set(const Image& texture)
    {
        VkDescriptorSet new_descriptor_set{};

        VkDescriptorSetAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = descriptor_pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &texture_descriptor_set_layout; //texture uniform layout

        ///Triangle descriptor sets
        descriptor_sets.tri_descriptor_set.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(vulkan_instance.device, &allocate_info, &new_descriptor_set) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        image_info.imageView = texture.image_view;
        image_info.sampler = texture.sampler;

        VkWriteDescriptorSet descriptor_write{};

        //Descriptor for the image sampler uniform
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = new_descriptor_set; //Binding to update
        descriptor_write.dstBinding = 1; //Binding index equal to shader binding index
        descriptor_write.dstArrayElement = 0; //Index of array data to update, no array so zero
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_write.descriptorCount = 1; //A single buffer info struct
        descriptor_write.pImageInfo = &image_info; //This is an image sampler so we provide an image data description

        //Apply updates, only write descriptor, no copy, so copy variable is 0
        vkUpdateDescriptorSets(vulkan_instance.device, 1, &descriptor_write, 0, nullptr);

        return new_descriptor_set;
    }

    void Vulkan_Engine::create_sync_objects()
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

    void Vulkan_Engine::start_record_command_buffer()
    {
        //Describe a new render pass targeting the given image index in the swapchain
        VkRenderPassBeginInfo render_pass_begin_info{};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

        //attach this render pass to the swap chain image
        render_pass_begin_info.renderPass = render_pass;
        render_pass_begin_info.framebuffer = swap_chain.framebuffers[current_image_index];

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

        if (vkBeginCommandBuffer(current_command_buffer, &begin_info) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }


        //VK_SUBPASS_CONTENTS_INLINE means we don't use secondary command buffers
        vkCmdBeginRenderPass(current_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);


        vkCmdSetViewport(current_command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(current_command_buffer, 0, 1, &scissor);
    }

    void Vulkan_Engine::end_record_command_buffer()
    {
        vkCmdEndRenderPass(current_command_buffer);

        if (vkEndCommandBuffer(current_command_buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    VkShaderModule Vulkan_Engine::create_shader_module(const std::vector<char>& bytecode)
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

    bool Vulkan_Engine::has_stencil_component(VkFormat format) const
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }
}