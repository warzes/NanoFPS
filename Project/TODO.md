﻿в физическом движке сделать так чтобы все созданные им ресурсы уничтожались при уничтожении физической системы. сейчас если забыть очистить, то будет краш


В настройках проекта - Advance - есть опция Unity (JUMBO) - попробовать перевести на нее - это создает автоматом unit cpp (то есть все cpp в одном файле)

https://github.com/skiriushichev/eely - система анимации (но только fbx)

https://github.com/gabime/spdlog - для лога
simdjson - для json
gainput - возможно для инпута
https://github.com/jrouwe/JoltPhysics - возможно вместо physx

вместо glfw - https://github.com/ColleagueRiley/RGFW


mastering-graphics-programming-vulkan - в этой книге много идей по дальнейшему развитию движка на вулкане (bindless ресурсы, создание pipeline из шейдер, многопоток, фреймграф с конфигурацией из json и т.д.)
	там же фрустум и оклюжен кулинг на гпу через мешлеты или выч шедйеры
	и кластер дефферед шейдинг, затенение, объемный туман


Кодестайл
где возможно, переписать
		[[nodiscard]] auto GetPxPhysics() { return m_physics; }