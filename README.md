This is an indie open-world RPG game developed by my sister and me.
It's created using Unreal Engine 5. 
  

The world is endless and auto-generated. It has 3 different biomes. I instantiate most HISMs via a PCG graph. The game has a system of roads and settlements.

![Untitled video - Made with Clipchamp (1)](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/be1c17e4-b6e3-4b71-bce2-5030f47b3b94)
![BiomesGeneration](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/877e6992-0e83-46fc-aa02-de3fa8636ea7)


The game has mechanics for fighting, crafting and researching, as well as capturing animals. There is some variety of creatures (both peaceful
and hostile), soon there will be more videos with their interaction. If you look closely at the video on the left, you will see how villagers run home when a monster appears

![Fight1](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/5dc754ad-abbd-4199-8199-ef70181fea99)
![Fighting2](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/0eb5efb0-3d9e-430d-b3bf-8970c87873f6)

(Outdated graphics) The fight starts automatically as soon as you get close enough to the enemy. Each creature has its own attack range, attack spread, and attack speed. Each creature in the area (puddle) is hit, allowing multiple targets to be hit at the same time. After being hit you are immune for a short time.

![Memoryator Preview  NetMode_ Standalone  (64-bit_SM5) 2023-05-07 13-10-14 (4)](https://github.com/GregoryBolshakov/GameSources/assets/19948668/d593b705-1611-40fe-ad35-fea88d4d8ae0)

(Outdated graphics) The inventory/drop system is simple, you can stack or split items you have, or those you can reach on the ground, or those in any other storrage, no matter where they are.

![Memoryator Preview  NetMode_ Standalone  (64-bit_SM5) 2023-05-15 19-57-33](https://github.com/GregoryBolshakov/GameSources/assets/19948668/5302f1f6-0c4b-4076-88cd-8e8864edb112)


![Memoryator Preview  NetMode_ Standalone  (64-bit_SM5) 2023-05-15 19-57-33 (1)](https://github.com/GregoryBolshakov/GameSources/assets/19948668/93713f82-4de4-4bbc-b0d9-1b937f2d9119)


The game uses Nakama server for authentication with platforms, transactions, friends and storing data. But Nakama doesn't implement transactions with Steam, I implemented the pipeline for microtransactions in a separate Go runtime module.


![Memoryator  DebugGame  - Unreal Editor 2023-06-12 19-33-55](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/cb41398f-5f91-4837-927d-1196e043590e)


You can also speak and trade with any mob, for instance, villagers. The communication widget is flexible, providing space for text and item exchange. This logic can be used not only for trades but also for quests, rewards, or stealing (similar to Fallout/Skyrim games). Improving your communication skills grants you better knowledge of the mob's items. Currently, they are completely hidden and represented by question marks. Mobs utilize the "knapsack" algorithm to determine the best counter-offer, but they have their own price coefficients for each item, which may vary depending on their attitude towards you or their disposition.
![Memoryator Preview  NetMode_ Standalone 0  (64-bit_D3D Shader Model 5) 2023-07-13 19-24-46](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/a8a64860-1cae-435a-b335-6d3eefbdef29)

![Memoryator Preview  NetMode_ Standalone 0  (64-bit_D3D Shader Model 5) 2023-07-13 19-24-46 (1)](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/1597f753-0748-4973-b223-81bb6fc8945e)
