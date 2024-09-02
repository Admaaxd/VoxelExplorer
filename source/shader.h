#pragma once

#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class shader
{
public:
	unsigned ID;

	shader(const char* vertexPath, const char* fragmentPath);
    ~shader();

	void use(); 
    void Delete();

	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, bool value) const;
	void setFloat(const std::string& name, bool value) const;

	void checkCompileErrors(unsigned shader, std::string type);

    void setVec2(const std::string& name, const glm::vec2& value) const;

    void setVec2(const std::string& name, float x, float y) const;

    void setVec3(const std::string& name, const glm::vec3& value) const;
 
    void setVec3(const std::string& name, float x, float y, float z) const;

    void setVec4(const std::string& name, const glm::vec4& value) const;

    void setVec4(const std::string& name, float x, float y, float z, float w) const;

    void setMat2(const std::string& name, const glm::mat2& mat) const;

    void setMat3(const std::string& name, const glm::mat3& mat) const;

    void setMat4(const std::string& name, const glm::mat4& mat) const;
    
};

#endif