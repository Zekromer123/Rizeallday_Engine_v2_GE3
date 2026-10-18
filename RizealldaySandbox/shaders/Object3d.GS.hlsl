#include "Object3d.hlsli"

[maxvertexcount(3)]
void main(triangle VertexShaderOutput input[3], inout TriangleStream<VertexShaderOutput> output)
{
    for (int i = 0; i < 3; ++i)
    {
        output.Append(input[i]);
    }
    output.RestartStrip();
}
