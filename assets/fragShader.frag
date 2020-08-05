#version 430 core 
precision mediump float; 

in vec4 passColor; 
out vec4 fragColor; 

void main(void) { 
   fragColor = passColor; 
};