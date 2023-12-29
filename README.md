# GameSources
This is an indie open-world RPG game developed by my sister and me.
It's created using Unreal Engine 5. 
  

The world is endless and auto-generated. It has 3 different biomes. Most of its objects are 2D but pretend to be 3D. You can also see their shadows don't rotate while the objects face the rotating camera.  
![Untitled video - Made with Clipchamp (1)](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/be1c17e4-b6e3-4b71-bce2-5030f47b3b94)
![BiomesGeneration](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/877e6992-0e83-46fc-aa02-de3fa8636ea7)


The game has mechanics for fighting, crafting and researching, as well as capturing animals. There is some variety of creatures (both peaceful
and hostile), soon there will be more videos with their interaction.

![Memoryator Preview  NetMode_ Standalone  (64-bit_SM5) 2023-05-07 13-10-14 (3)](https://github.com/GregoryBolshakov/GameSources/assets/19948668/6bd4c39a-35e2-431c-b943-7bea2d982989)

The fight starts automatically as soon as you get close enough to the enemy. Each creature has its own attack range, attack spread, and attack speed. Each creature in the area (puddle) is hit, allowing multiple targets to be hit at the same time. After being hit you are immune for a short time.

![Memoryator Preview  NetMode_ Standalone  (64-bit_SM5) 2023-05-07 13-10-14 (4)](https://github.com/GregoryBolshakov/GameSources/assets/19948668/d593b705-1611-40fe-ad35-fea88d4d8ae0)

The inventory/drop system is simple, you can stack or split items you have, or those you can reach on the ground, or those in any other storrage, no matter where they are.

![Memoryator Preview  NetMode_ Standalone  (64-bit_SM5) 2023-05-15 19-57-33](https://github.com/GregoryBolshakov/GameSources/assets/19948668/5302f1f6-0c4b-4076-88cd-8e8864edb112)


![Memoryator Preview  NetMode_ Standalone  (64-bit_SM5) 2023-05-15 19-57-33 (1)](https://github.com/GregoryBolshakov/GameSources/assets/19948668/93713f82-4de4-4bbc-b0d9-1b937f2d9119)


Rooarr! The villagers are afraid of us and scatter to their homes. 


![Memoryator Preview  NetMode_ Standalone  (64-bit_SM5) 2023-05-15 20-40-08](https://github.com/GregoryBolshakov/GameSources/assets/19948668/26b78e24-0603-4b76-9364-8e64417a9ed6)


Here, I am testing a full-fledged 3D environment created using assets I obtained through a giveaway.


![Memoryator  DebugGame  - Unreal Editor 2023-06-20 16-29-12 (online-video-cutter com) (1)](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/f9470a12-99fe-45a5-9360-d27712623727)


![Memoryator  DebugGame  - Unreal Editor 2023-06-20 16-29-12 (online-video-cutter com) (3)](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/7fb808a6-8fe8-4d5d-a3cf-685fb683083f)


The game uses Nakama server for authentication with platforms, transactions, friends and storing data. But Nakama doesn't implement transactions with Steam, I implemented the pipeline for microtransactions in a separate Go runtime module.


![Memoryator  DebugGame  - Unreal Editor 2023-06-12 19-33-55](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/cb41398f-5f91-4837-927d-1196e043590e)


You can also speak and trade with any mob, for instance, villagers. The communication widget is flexible, providing space for text and item exchange. This logic can be used not only for trades but also for quests, rewards, or stealing (similar to Fallout/Skyrim games). Improving your communication skills grants you better knowledge of the mob's items. Currently, they are completely hidden and represented by question marks. Mobs utilize the "knapsack" algorithm to determine the best counter-offer, but they have their own price coefficients for each item, which may vary depending on their attitude towards you or their disposition.
![Memoryator Preview  NetMode_ Standalone 0  (64-bit_D3D Shader Model 5) 2023-07-13 19-24-46](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/a8a64860-1cae-435a-b335-6d3eefbdef29)

![Memoryator Preview  NetMode_ Standalone 0  (64-bit_D3D Shader Model 5) 2023-07-13 19-24-46 (1)](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/1597f753-0748-4973-b223-81bb6fc8945e)
