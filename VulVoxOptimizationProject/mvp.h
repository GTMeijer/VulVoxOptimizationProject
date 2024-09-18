#pragma once

/// <summary>
/// Class representing the model, view, and projection matrices that transform objects from world space to clip space
/// </summary>
struct MVP
{
    glm::mat4 model; //Model space
    glm::mat4 view; //Camera space
    glm::mat4 projection; //Clip space
};