// Typedefs.
struct PS_Light {
    float3 pos;
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float constant;
    float lin;
    float quadratic;
    float intensity;
};

struct PS_Texture {
    float2 pos;
    float2 scale;
};
//#define N_POINTLIGHTS 8
//#define N_TEXS 8


// Used by vs.
cbuffer vs_constants : register(b0) {
    column_major float4x4 modelView; // Model matrix.
    column_major float4x4 projection;
    column_major float4x4 normMat;
};
// Used by ps.
cbuffer ps_constants : register(b0) {
    //PS_Texture tex[N_TEXS];
    PS_Texture tex[8];

    float registers[16];
};


cbuffer ps_light_constants : register(b1) {
    PS_Light dir_light;
    //PS_Light point_lights[N_POINTLIGHTS];
    PS_Light point_lights[8];

    float fog_min;
    float fog_max;
    float unused1;
    float unused2;
    float3 fog_color;
};
SamplerState my_sampler : register(s0);
texture2D base_texture : register(t0);
texture2D distortTexture : register(t1);


// Vertex attributes.
struct VertexInputType {
    float3 position: POSITION;

    float3 normal : NORMAL;
    float2 texCoords : TEXCOORDS;

    float4 color: COLOR;
};

struct PixelInputType {
    float4 projPosition: SV_POSITION; // REQUIRED.

    float3 viewPos : POSITION;

    float3 normal : NORMAL;
    float2 texCoords : TEXCOORDS;
    float4 color: COLOR;
};


// Vertex shader.
PixelInputType VS(VertexInputType input) {
    PixelInputType output;

    float4 pos = mul(modelView, float4(input.position, 1.0f));
    //output.position = mul(viewMatrix, output.position);
    //output.position = mul(projectionMatrix, output.position);

    output.projPosition = mul(projection, pos);
    //output.projPosition = float4(input.position, 1.0f);
    output.viewPos = pos.xyz;

    output.normal = mul(normMat, float4(input.normal, 0.0)).xyz;
    output.texCoords = input.texCoords;
    output.color = input.color;

    return output;
}


// Pixel shader. (I could do multiple targets in a single file, maybe)
float4 PS(PixelInputType input) : SV_TARGET {
    //return base_texture.Sample(my_sampler, input.texCoords) * input.color;
    float4 s = distortTexture.Sample(my_sampler, input.texCoords);
    float2 distortSample = s.rg + s.gb + s.ba + s.ar;

    float distortionStrength = 10000.0f;

    // The range [0.0, 1.0] would become [-0.5, -0.5].
    float2 offset = (distortSample - 0.5f) * 2.0f;
    float2 distortedUV = input.texCoords + (offset * distortionStrength);

    float4 finalColor = (base_texture.Sample(my_sampler, input.texCoords));
    finalColor = finalColor * input.color;
    //float4 finalColor = float4(1.0f, 1.0f, 0.0f, 1.0f);

    return finalColor;
}
