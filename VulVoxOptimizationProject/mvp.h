#pragma once

/// <summary>
/// Class representing the model, view, and projection matrices that transform objects from world space to clip space
/// </summary>
struct MVP
{
    glm::mat4 model{ 1.0f }; //Model space
    glm::mat4 view{ 1.0f }; //Camera space
    glm::mat4 projection{ 1.0f }; //Clip space
};