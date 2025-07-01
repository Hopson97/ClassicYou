#pragma once
#include <glad/glad.h>



struct WallProps
{
    GLuint texture_side_1;
    GLuint texture_side_2;
};

class Wall
{
  public:
  private:
    WallProps props_;
};