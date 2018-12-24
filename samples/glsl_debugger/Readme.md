This is simple example how to create GLSL debugger.</br>
In demo press `D` to run debugger for pixel under the cursor.</br>
The output will be stored in the `shader_debug_output.glsl` file.</br>
</br>
How it works:</br>
Glslang parse shader source and return AST, then injected some hooks on assign operators `= += -= *=` etc. In hook current pixel coordinate (gl_FragCoord.xy) compared with selected pixel (fs_Coord), it is needed to debug only one pixel. Hook input data and location in source writes into storage buffer `DebugOutputStorage` and will be processed by host. In this sample only fragment shader are supported.</br>
Patched shader code looks like that:
```cpp
layout(std430) buffer DebugOutputStorage
{
  uvec2	fs_Coord;
  
  uint	position;
  uint	outData [];
  
} dbgStorage;

bool IsCurrentShaderInvocation ()
{
  return (uint(gl_FragCoord.x) == dbgStorage.fs_Coord.x) && (uint(gl_FragCoord.y) == dbgStorage.fs_Coord.y);
}

float DebugDump (const float value, const uint sourceLocation)
{
  if ( IsCurrentShaderInvocation() )
  {
    uint	pos = atomicAdd( dbgStorage.position, 3 );
    dbgStorage.outData[pos++] = sourceLocation;
    dbgStorage.outData[pos++] = 0x100 | 0x1; // vector size & type id
    dbgStorage.outData[pos++] = floatBitsToUint( value );
  }
  return value;
}

void main ()
{
  float  value1 = ...
  float  value2 = DebugDump( value1, 0 ); // hook
  ...
}
```

And result will be:
```cpp
...
// float3{ -0.117698, 2.275348, -0.220623 }
202.   vec3 dist = abs(p) - radius;

// float1{ 2.290349 }
293. 		d = max(d, -sdBox(baseCenter - vec3(0.0, height+height2, 0.0), vec3(topWidth-0.0125, 0.015, topWidth-0.0125)));

// float3{ -0.000057, 2.235348, 0.083342 }
202.   vec3 dist = abs(p) - radius;

// float1{ 2.236902 }
301. 	d = min(d, sdBox(baseCenter - vec3((rand.x-0.5)*baseRad, height+height2, (rand.y-0.5)*baseRad),
302.                      vec3(baseRad*0.3*rand.z, 0.1*rand2.y, baseRad*0.3*rand2.x+0.025)));

// float3{ -0.007302, 2.290349, 0.004377 }
304.     vec3 boxPos = baseCenter - vec3((rand2.x-0.5)*baseRad, height+height2, (rand2.y-0.5)*baseRad);

// float1{ -1.000000 }
305.     float big = sign(boxPos.x);

// float1{ -0.074341 }
306.     boxPos.x = abs(boxPos.x)-0.02 - baseRad*0.3*rand.w;

// float3{ 0.012698, 2.282009, 0.015168 }
202.   vec3 dist = abs(p) - radius;

// float1{ 2.236902 }
307. 	d = min(d, sdBox(boxPos,
308.     vec3(baseRad*0.3*rand.w, 0.07*rand.y, baseRad*0.2*rand.x + big*0.025)));

// float2{ 2.236902, 0.000000 }
319.     vec2 distAndMat = vec2(d, 0.0);
...
```
