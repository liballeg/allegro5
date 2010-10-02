# define CAT(a, b)                      CAT_I(a, b)
# define CAT_I(a, b)                    CAT_II(a ## b)
# define CAT_II(res)                    res

# define JOIN1(a)                       a
# define JOIN2(a, b)                    CAT(a, b)
# define JOIN3(a, b, c)                 CAT(JOIN2(a, b), c)
# define JOIN4(a, b, c, d)              CAT(JOIN3(a, b, c), d)
# define JOIN5(a, b, c, d, e)           CAT(JOIN4(a, b, c, d), e)
# define JOIN6(a, b, c, d, e, f)        CAT(JOIN5(a, b, c, d, e), f)
# define JOIN7(a, b, c, d, e, f, g)     CAT(JOIN6(a, b, c, d, e, f), g)
# define JOIN8(a, b, c, d, e, f, g, h)  CAT(JOIN7(a, b, c, d, e, f, g), h)

# define DECL_VS_IN1(v0)                struct JOIN2(vs_input_, v0)               { _in_##v0                   }
# define DECL_VS_IN2(v0, v1)            struct JOIN4(vs_input_, v0, _, v1)        { _in_##v0 _in_##v1          }
# define DECL_VS_IN3(v0, v1, v2)        struct JOIN6(vs_input_, v0, _, v1, _, v2) { _in_##v0 _in_##v1 _in_##v2 }

# define DECL_VS_OUT1(v0)               struct JOIN2(vs_output_, v0)               { _out_##v0                     }
# define DECL_VS_OUT2(v0, v1)           struct JOIN4(vs_output_, v0, _, v1)        { _out_##v0 _out_##v1           }
# define DECL_VS_OUT3(v0, v1, v2)       struct JOIN6(vs_output_, v0, _, v1, _, v2) { _out_##v0 _out_##v1 _out_##v2 }

# define DEF_VS_BODY1(v0)               void JOIN2(vs_, v0)              (in JOIN2(vs_input_, v0)               i, out JOIN2(vs_output_, v0)               o) { _op_##v0                   }
# define DEF_VS_BODY2(v0, v1)           void JOIN4(vs_, v0, _, v1)       (in JOIN4(vs_input_, v0, _, v1)        i, out JOIN4(vs_output_, v0, _)            o) { _op_##v0 _op_##v1          }
# define DEF_VS_BODY3(v0, v1, v2)       void JOIN6(vs_, v0, _, v1, _, v2)(in JOIN6(vs_input_, v0, _, v1, _, v2) i, out JOIN6(vs_output_, v0, _, v1, _, v2) o) { _op_##v0 _op_##v1 _op_##v2 }

# define DEF_TECH1(v0)                  technique JOIN1(v0)               { pass { VertexShader = compile vs_1_1 JOIN2(vs_, v0)();               } }
# define DEF_TECH2(v0, v1)              technique JOIN3(v0, _, v1)        { pass { VertexShader = compile vs_1_1 JOIN4(vs_, v0, _, v1)();        } }
# define DEF_TECH3(v0, v1, v2)          technique JOIN5(v0, _, v1, _, v2) { pass { VertexShader = compile vs_1_1 JOIN6(vs_, v0, _, v1, _, v2)(); } }

# define DEF_VS1(v0)                    DECL_VS_IN1(v0);         DECL_VS_OUT1(v0);         DEF_VS_BODY1(v0)         DEF_TECH1(v0)
# define DEF_VS2(v0, v1)                DECL_VS_IN2(v0, v1);     DECL_VS_OUT2(v0, v1);     DEF_VS_BODY2(v0, v1)     DEF_TECH2(v0, v1)
# define DEF_VS3(v0, v1, v2)            DECL_VS_IN3(v0, v1, v2); DECL_VS_OUT3(v0, v1, v2); DEF_VS_BODY3(v0, v1, v2) DEF_TECH3(v0, v1, v2)

# define _in_pos0
# define _in_pos2                       float2 pos : POSITION;
# define _in_pos3                       float3 pos : POSITION;
# define _in_tex0
# define _in_tex2                       float2 tex : TEXCOORD0;
# define _in_col0
# define _in_col4_0                     float4 col : TEXCOORD0;
# define _in_col4_1                     float4 col : TEXCOORD1;

# define _out_pos0                      float4 pos : POSITION;
# define _out_pos2                      float4 pos : POSITION;
# define _out_pos3                      float4 pos : POSITION;
# define _out_tex0
# define _out_tex2                      float2 tex : TEXCOORD0;
# define _out_col0                      float4 col : COLOR0;
# define _out_col4_0                    float4 col : COLOR0;
# define _out_col4_1                    float4 col : COLOR0;

# define _op_pos0                       o.pos = float4(0.0f, 0.0f, 0.0f, 1.0f);
# define _op_pos2                       o.pos = mul(float4(i.pos.xy,   0.0f, 1.0f), g_world_view_proj);
# define _op_pos3                       o.pos = mul(float4(i.pos.xyz,        1.0f), g_world_view_proj);
# define _op_tex0
# define _op_tex2                       o.tex = mul(float4(i.tex.xy,   1.0f, 0.0f), g_texture_proj).xy;
# define _op_col0                       o.col = float4(1.0f, 1.0f, 1.0f, 1.0f);
# define _op_col4_0                     o.col = i.col;
# define _op_col4_1                     o.col = i.col;

float4x4 g_world_view_proj : register(c0);
float4x4 g_texture_proj    : register(c4);

DEF_VS3(pos3, tex2, col4_1);
DEF_VS3(pos3, tex2, col0);
DEF_VS3(pos3, tex0, col4_0);
DEF_VS3(pos3, tex0, col0);
DEF_VS3(pos2, tex2, col4_1);
DEF_VS3(pos2, tex2, col0);
DEF_VS3(pos2, tex0, col4_0);
DEF_VS3(pos2, tex0, col0);
DEF_VS3(pos0, tex2, col4_1);
DEF_VS3(pos0, tex2, col0);
DEF_VS3(pos0, tex0, col4_0);
DEF_VS3(pos0, tex0, col0);
