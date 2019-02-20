
#include "includes/Quad.h"
#include "includes/Engine.h"
#include <vector>
#include <iostream>

static const std::vector<GLfloat> quadData = std::vector<GLfloat>{
    // positions            normals                 texture coordinates
    -1.0f, -1.0f, 0.0f,     0.0f, 0.0f, -1.0f,		0.0f, 0.0f,
     1.0f, -1.0f, 0.0f,     0.0f, 0.0f, -1.0f,		1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,	    0.0f, 0.0f, -1.0f,		1.0f, 1.0f,
    -1.0f, -1.0f, 0.0f,     0.0f, 0.0f, -1.0f,      0.0f, 0.0f,
     1.0f,  1.0f, 0.0f,	    0.0f, 0.0f, -1.0f,		1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f,	    0.0f, 0.0f, -1.0f,		0.0f, 1.0f,
};
/*
static const std::vector<GLfloat> quadData = std::vector<GLfloat>{
        // positions            normals                 texture coordinates
        -1.0f, -1.0f, 0.0f,     0.0f, 0.0f, -1.0f,		0.0f, 0.0f,
        -1.0f, -1.0f, -1.0f,     0.0f, 0.0f, -1.0f,		1.0f, 0.0f,
        1.0f,  -1.0f, -1.0f,	    0.0f, 0.0f, -1.0f,		1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,     0.0f, 0.0f, -1.0f,      0.0f, 0.0f,
        1.0f,  1.0f, 0.0f,	    0.0f, 0.0f, -1.0f,		1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,	    0.0f, 0.0f, -1.0f,		0.0f, 1.0f,
};
 */

Quad::Quad() :
    RenderEntity(RenderProperties::FLAT) {
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_buffer);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _buffer);

    glBufferData(GL_ARRAY_BUFFER, quadData.size() * sizeof(float), &quadData[0], GL_STATIC_DRAW);

    // positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,   // attrib index
            3,                 // elems per attrib
            GL_FLOAT,          // type
            GL_FALSE,          // normalized
            8 * sizeof(float), // stride
            nullptr);          // initial offset

    // tex coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          2,
                          GL_FLOAT, GL_FALSE,
                          sizeof(float) * 8,
                          (void *)(sizeof(float) * 6));
    //glBindBuffer(GL_ARRAY_BUFFER, 0);

    // normals
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(float) * 8,
            (void *)(sizeof(float) * 3));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Quad::~Quad() {
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_buffer);
}

void Quad::render() {
    glDisable(GL_CULL_FACE);
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}
