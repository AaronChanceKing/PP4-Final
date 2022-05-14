#pragma once
#include <string>

namespace Shaders
{
	const char* vertexShader = R"(
// an ultra simple hlsl vertex shader
// TODO: Part 2b
[[vk::push_constant]] 
cbuffer SHADER_VAR
{
	matrix wm;
	matrix vm;
};

// TODO: Part 2f, Part 3b
// TODO: Part 1c
float4 main(float4 inputVertex : POSITION) : SV_POSITION
{
	// TODO: Part 2d
	inputVertex = mul(wm, inputVertex);
	// TODO: Part 2f, Part 3b
	inputVertex = mul(vm, inputVertex);
	return inputVertex;
}
)";
	// Simple Pixel Shader
	const char* pixelShader = R"(
// an ultra simple hlsl pixel shader
float4 main() : SV_TARGET 
{	
	return float4(0.9f ,0.8f, 0.01f, 0); // TODO: Part 1a
}
)";
}