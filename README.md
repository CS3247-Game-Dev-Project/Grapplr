# Grapplr

Grapplr is a roguelike beat'em'up style game, taking inspirations from games like Vampire Survivors and Megabonk.

Players fend off waves of enemies growing in difficulty and size, pick up items, and level up to grow stronger.

Equipped with the player is a handy grappling hook that helps them traverse through a medieval-style city.

## Progress through the game to slay hordes of enemies... till none remains.

### Features

- Skill based grappling hook movement to navigate and evade enemies
- Handcrafted map layout and environment design
- 5 unique item types to find and level up
- 3 unique enemy movement styles for a total of 10 enemies types including a mini-boss
- Fight against large hordes of enemies that keep on spawning

---

### Game Design

Dynamics

- Risk vs reward (fight vs retreat for HP)
- Early game → Late game escalation (hordes, mini-bosses)
- Positive mastery loop (skill improves each run)
- Positive feedback loop (kills → upgrades →stronger build)
- Constant repositioning to avoid being surrounded

Aesthetics (Player Experience)

- Increasing tension as enemies swarm from all directions
- Sense of challenge and survival under pressure
- Satisfaction from defeating large hordes
- Feeling of mastery and improvement over time

#### Level Design

- Map is designed as a dense urban city/town, where players can move through streets and rooftops
- The level is built with multiple vertical layers
  - Street level where most items will spawn
  - Rooftops to escape land enemies
  - Towers to grapple to height and traverse large distances

Because traversal is central to the game, the city layout prioritizes:

- Wide enough streets for easy grappling and camera movement
- Rooftop spacing for chaining grapples
- Height variation for momentum and route choices
- Open areas for large fights and loot moments
- Narrower alleyways that the player have to navigate

Buildings were created with modular assets (modular static meshes for walls and roofs)

#### Enemy Design

Balancing

- Damage scales additively, health scales multiplicatively -> ensures that enemy stats grow faster than the player so that game becomes more challenging as time passes
- Flying enemies are slightly faster than the player to encourage the use of the grappling hook to escape
- Climbing/flying enemies prevent the player from escaping to the rooftops

Waves/Enemy Spawning

- New waves trigger upon a certain percentage of enemy kills or after a set period of time
- Loop through different enemy types on each wave, each enemy type has varying base stats
- Mini-boss every 10 waves -> serves as a challenge to ensure the player has levelled up enough

Interactions

- On taking damage, enemies will
  - Flash white to show positive hit feedback (certain enemies do not flash due to constraint of time)
  - Trigger a sound cue (concurrent cues are capped at 8)
- Used damage numbers instead of health bars to declutter screen
- Shows increases in item damage
- Damage number pops up on hit, size adjusted on damage amount, so that bigger hits feel more impactful

#### Grapple Design

Overview

- Main movement mechanic of the character
  Shoots a grapple hook and launches player in the direction where grapple hook latches
- Used to escape enemies, move through map fast, kill flying enemies, or dash through enemies while attacking

Technical Implementation

- RMB triggers the shoot of a grapple hook projectile if the reticle is aimed at a valid grapple point (detected via sphere trace)
- The grapple hook projectile is subject to slight gravity for realistic feel
- After hook is latched, the player gets launched towards the hook latch point
- The speed of the launch is log scaled by the distance b/w the player and the grapple point (more distance = higher speed)
- Player can stop grappling mid-grapple by pressing space. Conserves 60% momentum. Helps player feel in control

---

### Technical Design

Mass Entity

- Supports multiple level of details (LODs) via the Mass Moveable Visualization Trait.
  - Use actors for high/medium LOD entities (30-50 depending on complexity of enemy mesh)
  - Use static meshes for low LOD entities (static meshes allow for better rendering performance)
- Syncs actor movement from Mass Entity via the Agent Capsule Collision Sync Trait in the Mass Agent component in the actor blueprint.

#### 2.5D Flow Field in Mass Entity

- Ground enemies can pathfind around buildings and obstacles via the flow field
- Running A\* is too slow given the number of ground enemies. Flow Field optimizes pathfinding performance for ground enemies.
- Used in the ground enemy movement processor to generate their initial forward vectors

#### Signed Distance Field (SDF) in Mass Entity

- Used for enemy wall avoidance, reduces enemy wall clipping.
- Used in our movement style (ground/climbing/flying) processors, by adjusting the forward vector using a repulsion force based on the SDF gradient from the closest wall.
- Works with the flow field for ground enemies.

#### 3D Spatial Hash Grid in Mass Entity

- Used to generate and apply avoidance forces, which spread out enemies more evenly, prevent them from overlapping (to enhance game feel)
- Checking through every enemy, then applying avoidance is too slow
  Spatial Hash Grid optimizes the check to only nearby enemies
- Avoidance forces are applied after the movement styles (after Flow Field and SDF forces)

#### State Tree with Mass Entity

- A variant of State Machine/Behaviour Tree but for Mass Entity
- Simple state tree with three states: Move/Attack/Death
- Keep more finer decision logic out of the state tree to optimize performance, since state tree doesn’t access entities by chunks

---

### Credits

Made by NUS AY25/26 CS3247 Group 2:

- Aaron Sim
- Ito Tetsushi
- Peng Victor M X
- Que Linxiao
- Tayal Yonish
- Titus Chew Xuan Jun

---

#### Asset Credits (non-exhaustive)

**3D Models**

- Rogue Character Model: https://www.fab.com/listings/15e2afd1-11c0-4679-832c-ebbfb6162583
- Minion Flying: https://sketchfab.com/3d-models/minion--flying-99ebd5f79d01411fa2f1a7da0cfcea30
- Quadruped Fantasy Creatures: https://www.fab.com/listings/52d686b6-1180-4f26-901f-ce3c69a14767
- Deep Hunter: https://www.fab.com/listings/77903f1c-2618-43bd-aba3-0addb8b6685c
- Hell Drake: https://sketchfab.com/3d-models/hell-drake-c8505007238d44bcb575c7763307a570
- Infernal Magma Hound: https://sketchfab.com/3d-models/infernal-magma-hound-free-lava-creature-asset-bd8290074df34eb99f174edb176fcc46
- Magic Gemme: https://www.fab.com/listings/91866921-0cbb-4f78-9a1a-de50a63a4695

**3D Environment Models**

- Quixel Megasacans
- Cinematic Plans and Mountains: https://www.fab.com/listings/d09b37b5-35d1-44ba-8059-a45c44cf1d5b
- Megaplants: Greasewood: https://www.fab.com/listings/1b8d0a76-276f-406e-9536-330243bb42cf
- Quick TreeIt Tree: https://www.fab.com/listings/069a0d0a-bd04-4664-ba5b-14df2c79720c
- Tower and Castle Walls: https://www.fab.com/listings/bed12176-8829-4786-9569-fc23ca76ff70
- Medieval Food Stall: https://www.fab.com/listings/9f133e9d-a585-49c3-9dff-be7070a1e186
- Medieval Market Barrel: https://www.fab.com/listings/01ab63de-eab2-4ecc-a786-b6a8ac7a2237
- Medieval Market Rug and Crates: https://www.fab.com/listings/c5081654-9187-47fa-984d-a7e57413f0a5
- Medieval Supply Wagon: https://www.fab.com/listings/2887998a-b561-431d-bb9b-7ac39a90aa39

**Audio Assets**

- Epic Dragon Roar: https://pixabay.com/sound-effects/film-special-effects-epic-dragon-roar-364481/
- Dragon Wings Flapping: https://pixabay.com/sound-effects/film-special-effects-dragon-wings-flapping-478385/
- Dragon Death: https://pixabay.com/sound-effects/horror-dragon-death-102428/
- Whoosh Effect: https://pixabay.com/sound-effects/film-special-effects-whoosh-effect-382717/
- Wind Gust: https://pixabay.com/sound-effects/nature-wind-gust-386158/
- Dramatic Synth Echo: https://pixabay.com/sound-effects/film-special-effects-dramatic-synth-echo-43970/
- Slow Wind Sound Effect: https://pixabay.com/sound-effects/film-special-effects-slow-wind-sound-effect-108401/
- Metallic Latch Attach Release: https://pixabay.com/sound-effects/film-special-effects-metallic-attach-release-35932/
- Metallic Latch Release: https://pixabay.com/sound-effects/film-special-effects-metallic-latch-release-43678/
- Game Bonus 03: https://pixabay.com/sound-effects/technology-game-bonus-03-487857/

**Font Assets**

- Enchanted Land Font: https://www.dafont.com/enchanted-land.font
- Dash Horizon Font: https://www.dafont.com/dash.font
