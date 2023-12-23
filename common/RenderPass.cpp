#include "RenderPass.h"
#include <string>
#include <glad/glad.h>
#include <glfw/glfw3.h>

void RenderPass::BindData(bool finalPass) {
	if (!finalPass) {
		glGenFramebuffers(1, &fbo);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	float quadVertices[] = {
		//positions   //texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	if (!finalPass) {
		std::vector<unsigned int> attachments;
		for (int i = 0; i < colorAttachments.size(); i++) {
			glBindTexture(GL_TEXTURE_2D, colorAttachments[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorAttachments[i], 0);
			attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
		}
		glDrawBuffers(attachments.size(), &attachments[0]);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass::Draw(std::vector<unsigned int> texPassArray) {
	glUseProgram(program);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	for (int i = 0; i < texPassArray.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, texPassArray[i]);
		std::string uName = "texPass" + std::to_string(i);
		glUniform1i(glGetUniformLocation(program, uName.c_str()), i);
	}
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}