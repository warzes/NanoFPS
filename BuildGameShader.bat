dxc -spirv -T vs_6_6 -E vsmain "bin\GameData\Shaders\DiffuseShadow.hlsl" -Fo "bin\GameData\Shaders\spv\DiffuseShadow.vs.spv" -fspv-preserve-interface -fspv-target-env="vulkan1.3" -fvk-use-dx-layout
dxc -spirv -T ps_6_6 -E psmain "bin\GameData\Shaders\DiffuseShadow.hlsl" -Fo "bin\GameData\Shaders\spv\DiffuseShadow.ps.spv" -fspv-preserve-interface -fspv-target-env="vulkan1.3" -fvk-use-dx-layout

dxc -spirv -T vs_6_6 -E vsmain "bin\GameData\Shaders\Depth.hlsl" -Fo "bin\GameData\Shaders\spv\Depth.vs.spv" -fspv-preserve-interface -fspv-target-env="vulkan1.3" -fvk-use-dx-layout