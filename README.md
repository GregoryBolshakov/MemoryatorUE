This is a multiplayer game inspired by Minecraft and Don't Starve created using Unreal Engine 5.


The world is infinite and procedurally generated. It has 3 different biomes. Repeated objects like trees and grass are effectively rendered in a single draw call. The game has a system of roads and settlements.

![WorldExplore](https://github.com/user-attachments/assets/b8c6c534-af24-4205-837e-2019ba2fb7f0)
![ChunkSystem](https://github.com/user-attachments/assets/0dc94780-fb81-4d95-848e-90c4a113c7e8)


You can speak and trade with any mob, for example, villagers. The communication is based on OpenAI API. Responses are streamed back in portions to speed it up.
This mechanic is currently in a WIP state. The plan is not only use LLM for text responses, but for behavior tree management. I store the context between each pair of characters that affect the decision making (NPC may immediately attack you, or give some gifts, etc.). As a context, each NPC stores 10 of the most important events that happened between them and you (and other NPCs).

Events can be either ```communicative```: a deal, an insult, a compliment, intelligence, a lie;
or ```active```: a fight, damage to property, a favor

![TradeAndCommunication](https://github.com/user-attachments/assets/017728df-0813-4bca-8cde-c699a56cf132)


The trade is done using knapsack algorithm. NPCs find the best counter-offer, but they have their own price coefficients for each item, which may depend on their attitude to you or other factors.
They always offer items they have, you can see them as question marks. Part of a mini game is to figure out what they have by offering different items. Some mechanics and skills will allow to see hidden items directly.

![TradeOnly+HighlightSecrets](https://github.com/user-attachments/assets/339c3c9a-c0b7-46e4-b7e0-9c7bcf0c254f)


(Old graphics) There are three relationship types: `Neutral`, `Friendly`, and `Hostile`. When you approach a `Hostile` mob, you will automatically start attacking it. Combat is designed to be WoW-like, with the option to use abilities while the basic fight depends on stats and calculations. You can also exploit distances and speeds to gain tactical advantages, as shown in the video.
There is a couple multiplayer bugs for replicated characters puddles right now.

Not everyone attacks when approached. You can see the villagers running home when they see the monster.

![Fight1](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/5dc754ad-abbd-4199-8199-ef70181fea99)
![Fighting2](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/0eb5efb0-3d9e-430d-b3bf-8970c87873f6)

(Old graphics) Each creature has its own attack range, attack spread angle, and attack speed. Each creature in the area (puddle) is hit, allowing multiple targets to be hit at the same time. After being hit you are immune for a short time. The immunity is a part of a bigger `buff system`. It can handle states like `poisoned`, `shielded`, `damage increased`, etc.

![Memoryator Preview  NetMode_ Standalone  (64-bit_SM5) 2023-05-07 13-10-14 (4)](https://github.com/GregoryBolshakov/GameSources/assets/19948668/d593b705-1611-40fe-ad35-fea88d4d8ae0)


I made a tool to spread buildings on a perimeter of a circle. Each outpost can have custom circles like trading area with stalls or blacksmiths' quarter, magicians' quarter, etc. I can also set Min/Max numbers for specific buildings and all the details of their occupants.
While I could support more shapes than just circles, I do plan to move village generation to the Unreal PCG plugin in the future.

![VillageGeneration](https://github.com/user-attachments/assets/08448c78-6956-49d6-85a9-6e97c19ac3d5)
![VillageGeneration2](https://github.com/user-attachments/assets/0589a6c7-6e04-4746-8cd8-31ebba1f6880)


The inventory/drop system is simple, you can stack or split items you have, or those you can reach on the ground, or those in any other storrage, no matter where they are.
All inventries are server-authorized. I created an Inventory Controller class for players to request putting/taking/throwing away stuff via Server/Client RPCs.

![InventoryMultiplayer](https://github.com/user-attachments/assets/a485a993-daa4-4da5-b9f7-3aeb2404c532)


(Old graphics) The game uses Nakama server for authentication with platforms, transactions, friends and storing data. But Nakama doesn't implement transactions with Steam, I implemented the pipeline for microtransactions in a separate Go runtime module.

![Memoryator  DebugGame  - Unreal Editor 2023-06-12 19-33-55](https://github.com/GregoryBolshakov/MemoryatorUE/assets/19948668/cb41398f-5f91-4837-927d-1196e043590e)

