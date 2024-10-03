/*
 * cartridge.c - cartridge emulation
 *
 * Copyright (C) 2001-2003 Piotr Fusik
 * Copyright (C) 2001-2005 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <string/stdstring.h>
#include <utils/md5.h>

#include "atari.h"
#include "cartridge.h"
#include "memory.h"
#include "pia.h"
#include "rtime8.h"
#include "util.h"
#include "statesav.h"

extern void a5200_log(enum retro_log_level level, const char *format, ...);

static const struct cart_info_t cart_info_none = {
	"",
	CART_NONE,
	1,
	1.0f,
	1.0f,
	""
};

static const struct cart_info_t cart_info_table[] = {
	{ "75925ce4174594735832c2de30e8ffde", CART_5200_32,    1,  1.0f,  1.0f, "3-D Tic-Tac-Toe (Conversion)" },
	{ "90dee6cf40d5902f199c575bbc246bce", CART_5200_32,    1,  1.0f,  1.0f, "Abracadabra! (Conversion)" },
	{ "1c672fe917f11f7499e655a3fe485636", CART_5200_32,    1,  1.0f,  1.0f, "Analog Classics #1 (Conversion)" },
	{ "72a91c53bfaa558d863610e3e6d50213", CART_5200_NS_16, 1,  1.0f,  1.0f, "Ant Eater" },
	{ "e295b1a01ce0f17a9ecb68a7ba384ff9", CART_5200_NS_16, 1,  1.0f,  1.0f, "Ant Eater (Alternate)" },
	{ "c8e90376b7e1b00dcbd4042f50bffb75", CART_5200_NS_16, 1,  1.0f,  1.0f, "Atari 5200 Calibration Cart" },
	{ "45f8841269313736489180c8ec3e9588", CART_5200_NS_16, 1,  1.0f,  1.0f, "Activision Decathlon, The (USA)" },
	{ "4b1aecab0e2f9c90e514cb0a506e3a5f", CART_5200_32,    1,  1.0f,  1.0f, "Adventure II-a" },
	{ "e2f6085028eb8cf24ad7b50ca4ef640f", CART_5200_32,    1,  1.0f,  1.0f, "Adventure II-b" },
	{ "b48dd725b5d024ef0a5a797fb5acefc6", CART_5200_32,    1,  1.0f,  1.0f, "Alien Swarm (XL Conversion)" },
	{ "9e6d04dc20cbd6d3cdb722e420dea203", CART_5200_32,    1,  1.0f,  1.0f, "ANALOG Multicart (XL Conversion)" },
	{ "737717ff4f8402ed5b02e4bf866bbbe3", CART_5200_32,    1,  1.0f,  1.0f, "ANALOG Multicart V2 (XL Conversion)" },
	{ "77c6b647746bb1413c5566378ef25eec", CART_5200_32,    1,  1.0f,  1.0f, "Archon (XL Conversion)" },
	{ "496a4cdb681e938f6cca13ac679fff7f", CART_5200_32,    1,  1.0f,  1.0f, "Archon (Alternate)" },
	{ "bae7c1e5eb04e19ef8d0d0b5ce134332", CART_5200_EE_16, 1,  1.0f,  1.0f, "Astro Chase (USA)" },
	{ "10cdf2bbb058bb4cc518fd25031f427d", CART_5200_32,    1,  1.0f,  1.0f, "Astro Grover (XL Conversion)" },
	{ "d31a3bbb4c99f539f0d2c4e02bec516e", CART_5200_32,    1,  1.0f,  1.0f, "Atlantis (XL Conversion)" },
	{ "ec65389cc604b279d69a889725c723e7", CART_5200_32,    1,  1.0f,  1.0f, "Attack of the Mutant Camels (XL Conversion)" },
	{ "eadcaf6e7b09a2350a10ac2c5b6c776a", CART_5200_8,     1,  1.0f,  1.0f, "Bacterion (Conversion)" },
	{ "f5cd178cbea0ae7d8cf65b30cfd04225", CART_5200_32,    1,  0.7f,  1.0f, "Ballblazer (USA)" },
	{ "c9019124080eccb37c20ecf539d72796", CART_5200_NS_16, 1,  1.0f,  1.0f, "Basketball (Conversion)" },
	{ "8576867c2cfc965cf152be0468f684a7", CART_5200_EE_16, 1,  1.0f,  1.0f, "Battlezone (1983) (Atari) (Prototype)" },
	{ "96b424d0bb0339f4edfe8095fe275d62", CART_5200_32,    1,  1.0f,  1.0f, "Batty Builders (XL Conversion)" },
	{ "8123393ae9635f6bc15ddc3380b04328", CART_5200_NS_16, 1,  1.0f,  1.0f, "Blueprint (1982) (CBS)" },
	{ "17e5c03b4fcada48d4c2529afcfe3a70", CART_5200_32,    1,  1.0f,  1.0f, "BCs Quest For Tires (XL Conversion)" },
	{ "96ec5b299b203c88f98100b57af6838d", CART_5200_32,    1,  1.0f,  1.0f, "Biscuits From Hell" },
	{ "315e0bb45f28bb227e92b8c9e00ee8eb", CART_5200_32,    1,  1.0f,  1.0f, "Blaster" },
	{ "992f62ccfda4c92ef113af1dd96d8f55", CART_5200_NS_16, 1,  1.0f,  1.0f, "BlowSub" },
	{ "1913310b1e44ad7f3b90aeb16790a850", CART_5200_NS_16, 1,  1.0f,  1.0f, "Beamrider (USA)" },
	{ "f8973db8dc272c2e5eb7b8dbb5c0cc3b", CART_5200_NS_16, 1,  1.0f,  1.0f, "BerZerk (USA)" },
	{ "139229eed18032fdea735fa5360bd551", CART_5200_32,    1,  1.0f,  1.0f, "Beef Drop Ultimate SD Edition" },
	{ "81790daff7f7646a6c371c056622be9c", CART_5200_40,    1,  1.0f,  1.0f, "Bounty Bob Strikes Back (Merged) (Big Five Software) (U)" },
	{ "5d5a75ac53230915701bc918249f1055", CART_5200_40_ALT,1,  1.0f,  1.0f, "Bounty Bob Strikes Back! (1984)(Big Five Software)(US)" },
	{ "a074a1ff0a16d1e034ee314b85fa41e9", CART_5200_EE_16, 1,  1.0f,  1.0f, "Buck Rogers - Planet of Zoom (USA)" },
	{ "f345dd189879f8d13e4c8c5737279fa3", CART_5200_EE_16, 1,  1.0f,  1.0f, "Buck Rogers - Planet of Zoom (USA)[a]" },
	{ "713feccd8f2722f2e9bdcab98e25a35f", CART_5200_32,    1,  1.0f,  1.0f, "Buried Bucks (XL Conversion)" },
	{ "3147ad22f8d5f46b1ef40a39da3a3de1", CART_5200_32,    1,  1.0f,  1.0f, "Captain Beeble (XL Conversion)" },
	{ "79335deb06a1ef532fea8eee8012ecde", CART_5200_NS_16, 1,  1.0f,  1.0f, "Capture the Flag" },
	{ "01b978c3faf5d516f300f98c00377532", CART_5200_8,     1,  1.0f,  1.0f, "Carol Shaw's River Raid (USA)" },
	{ "4965b4c8acca64c4fac39a7c0763f611", CART_5200_32,    1,  1.0f,  1.0f, "Castle Blast (USA) (Unl)" },
	{ "8f4c07a9e0ef2ded720b403810220aaf", CART_5200_32,    1,  1.0f,  1.0f, "Castle Crisis (USA) (Unl)" },
	{ "d64a175672b6dba0c0b244c949799e64", CART_5200_32,    1,  1.0f,  1.0f, "Caverns of Mars (Conv)" },
	{ "1db260d6769bed6bf4731744213097b8", CART_5200_NS_16, 1,  1.0f,  1.0f, "Caverns Of Mars 2 (Conv)" },
	{ "c4a14a88a4257970223b1ef9bf95da5b", CART_5200_NS_16, 1,  1.0f,  1.0f, "Caverns Of Mars 3 (Conv)" },
	{ "261702e8d9acbf45d44bb61fd8fa3e17", CART_5200_EE_16, 1, 0.17f,  0.5f, "Centipede (USA)" },
	{ "df283efab9d36a15603283ee2a7bdb71", CART_5200_32,    1,  1.0f,  1.0f, "Chess (XL Conversion)" },
	{ "21b722b9c93076a3605ec157ac3aa4b8", CART_5200_32,    1,  1.0f,  1.0f, "Chop Suey" },
	{ "3ff7707e25359c9bcb2326a5d8539852", CART_5200_NS_16, 1,  1.0f,  1.0f, "Choplifter! (USA)" },
	{ "701dd2903b55a5b6734afa120e141334", CART_5200_32,    1,  1.0f,  1.0f, "Chicken (XL Conversion)" },
	{ "e60a98edcc5cad98170772ea8d8c118d", CART_5200_32,    1,  1.0f,  1.0f, "Claim Jumper (XL Conversion)" },
	{ "f21a0fb1653215bbeea87dd80249015e", CART_5200_NS_16, 1,  1.0f,  1.0f, "Claim Jumper (XL Converion Alternate)" },
	{ "4a754460e43bebd08b943c8dba31d581", CART_5200_32,    1,  1.0f,  1.0f, "Clowns & Balloons (XL Conversion)" },
	{ "dc382809b4ba707d8a9084421c7a4976", CART_5200_NS_16, 1,  1.0f,  1.0f, "Cloudburst" },
	{ "5720423ebd7575941a1586466ba9beaf", CART_5200_EE_16, 1,  1.0f,  1.0f, "Congo Bongo (USA)" },
	{ "88ea120ef17747d7b567ffa08b9fb578", CART_5200_EE_16, 1,  1.0f,  1.0f, "Congo Bongo (1983) (Sega)" },
	{ "1a64edff521608f9f4fa9d7bdb355087", CART_5200_EE_16, 1,  1.0f,  1.0f, "Countermeasure (USA)" },
	{ "4c034f3db0489726abd401550a402c32", CART_5200_32,    1,  1.0f,  1.0f, "COSMI (XL Conversion)" },
	{ "195c23a894c7ac8631757eec661ab1e6", CART_5200_32,    1,  1.0f,  1.0f, "Crossfire (XL Conversion)" },
	{ "cd64cc0b348a634080078206e3111f9a", CART_5200_32,    1,  1.0f,  1.0f, "Crystal Castles (Final Conversion)" },
	{ "550ef46dfa0945b6dd693b0fab6385a5", CART_5200_32,    1,  1.0f,  1.0f, "Crystal Castles (Clay)" },
	{ "c24be906c9d79f4eab391fd583332a4c", CART_5200_32,    1,  1.0f,  1.0f, "Curse of the Lost Miner" },
	{ "7c27d225a13e178610babf331a0759c0", CART_5200_NS_16, 1,  1.0f,  1.0f, "David Crane's Pitfall II - Lost Caverns (USA)" },
	{ "27d5f32b0d46d3d80773a2b505f95046", CART_5200_EE_16, 1,  1.0f,  1.0f, "Defender (1982) (Atari)" },
	{ "8e280ad05824ef4ca32700716ef8e69a", CART_5200_NS_16, 1,  1.0f,  1.0f, "Deluxe Invaders" },
	{ "b4af8b555278dec6e2c2329881dc0a15", CART_5200_32,    1,  1.0f,  1.0f, "Demon Attack (XL Conversion)" },
	{ "32b2bb28213dbb01b69e003c4b35bb57", CART_5200_32,    1,  1.0f,  1.0f, "Desmonds Dungeon (XL Conversion)" },
	{ "6049d5ef7eddb1bb3a643151ff506219", CART_5200_32,    1,  1.0f,  1.0f, "Diamond Mine (XL Conversion)" },
	{ "3abd0c057474bad46e45f3d4e96eecee", CART_5200_EE_16, 1,  1.0f,  1.0f, "Dig Dug (1983) (Atari)" },
	{ "2108ee2967b6cd7ceda757a601c4b63c", CART_5200_32,    1,  1.0f,  1.0f, "Dig Dug 10-9 (Conversion)" },
	{ "1d1eab4067fc0aaf2b2b880fb8f72e40", CART_5200_32,    1,  1.0f,  1.0f, "Donkey Kong Arcade" },
	{ "4dcca2e6a88d57e54bc7b2377cc2e5b5", CART_5200_32,    1,  1.0f,  1.0f, "Donkey Kong Jr Enhanced" },
	{ "0c393d2b04afae8a8f8827d30794b29a", CART_5200_32,    1,  1.0f,  1.0f, "Donkey Kong (XL Conversion)" },
	{ "ae5b9bbe91983ab111fd7cf3d29d6b11", CART_5200_32,    1,  1.0f,  1.0f, "Donkey Kong Jr (XL Conversion)" },
	{ "159ccaa564fc2472afd1f06665ec6d19", CART_5200_8,     1,  1.0f,  1.0f, "Dreadnaught Factor, The (USA)" },
	{ "b7fafc8ae6bb0801e53d5756b14dbe31", CART_5200_NS_16, 1,  1.0f,  1.0f, "Drelbs (Conversion)" },
	{ "ec325297c45973292a9e99e807d3efdf", CART_5200_32,    1,  1.0f,  1.0f, "Ducks Ahoy (Conversion)" },
	{ "2d9e5f9589e6162905cfd164ee41a13a", CART_5200_NS_16, 1,  1.0f,  1.0f, "Embargo (Conversion)" },
	{ "e9b7d19c573a30e6503f35c886666358", CART_5200_32,    1,  1.0f,  1.0f, "Encounter" },
	{ "7259353c39aadf76f2eb284e7666bb58", CART_5200_32,    1,  1.0f,  1.0f, "ET (32k)" },
	{ "5789a45479d9769d4662a15f349d83ed", CART_5200_32,    1, 0.12f,  0.3f, "Fairy Force (homebrew)" },
	{ "4b6c878758f4d4de7f9650296db76d2e", CART_5200_32,    1,  1.0f,  1.0f, "Fast Eddie (XL Conversion)" },
	{ "cc9a9240975aceb5953996b1ed1b88c6", CART_5200_32,    1,  1.0f,  1.0f, "Fast Eddie (Alternate)" },
	{ "5cf2837752ef8dfa3a6962a28fc0077b", CART_5200_8,     1,  1.0f,  1.0f, "Falcon (XL Conversion)" },
	{ "6b58f0f3175a2d6796c35afafe6b245d", CART_5200_8,     1,  1.0f,  1.0f, "Floyd The Droid (XL Conversion)" },
	{ "14bd9a0423eafc3090333af916cfbce6", CART_5200_EE_16, 1,  1.0f,  1.0f, "Frisky Tom (USA) (Proto)" },
	{ "c717ebc92233d206f262d15258e3184d", CART_5200_EE_16, 1,  1.0f,  1.0f, "Frisky Tom (USA) (Hack)" },
	{ "598b469a04c956a5c1e15827773a4a8b", CART_5200_32,    1,  1.0f,  1.0f, "Forbidden Forest - Slinky (Conversion)" },
	{ "05a086fe4cc3ad16d39c3bc45eb9c26f", CART_5200_32,    1,  1.0f,  1.0f, "Fort Apocalypse (XL Conversion)" },
	{ "a8ee72c80491ce33e24b7f56c2865a27", CART_5200_32,    1,  1.0f,  1.0f, "Freecell XE (Conversion)" },
	{ "2c89c9444f99fd7ac83f88278e6772c6", CART_5200_8,     1,  1.0f,  1.0f, "Frogger (1983) (Parker Bros)" },
	{ "d8636222c993ca71ca0904c8d89c4411", CART_5200_EE_16, 1,  1.0f,  1.0f, "Frogger II - Threeedeep! (USA)" },
	{ "8e03034fc42ceaa5f7688af08c1f077c", CART_5200_EE_16, 1,  1.0f,  1.0f, "Frogger II - Threeedeep! (US)[a]" },
	{ "d1a3b6613b03716af6aefe21d1bfdf07", CART_5200_EE_16, 1,  1.0f,  1.0f, "Frogger II - Threeedeep! (US)[o2][a]" },
	{ "98113c00a7c82c83ee893d8e9352aa7a", CART_5200_8,     1,  1.0f,  1.0f, "Galactic_Chase" },
	{ "3ace7c591a88af22bac0c559bbb08f03", CART_5200_8,     1,  1.0f,  1.0f, "Galaxian (1982) (Atari)" },
	{ "4012282da62c0d72300294447ef6b9a2", CART_5200_32,    1,  1.0f,  1.0f, "Gateway to Apshai (XL Conversion)" },
	{ "0fdce0dd4014f3188d0ca289f53387d0", CART_5200_32,    1,  1.0f,  1.0f, "Gebelli Compilation (XL Conversion)" },
	{ "85fe2492e2945015000272a9fefc06e3", CART_5200_8,     1,  1.0f,  1.0f, "Gorf (1982) (CBS)" },
	{ "a21c545a52d488bfdaf078d786bf4916", CART_5200_32,    1,  1.0f,  1.0f, "Gorf Converted (1982) (CBS)" },
	{ "dc271e475b4766e80151f1da5b764e52", CART_5200_32,    1,  1.0f,  1.0f, "Gremlins (USA)" },
	{ "af6a7429a39039d04acf69f0cd6321a2", CART_5200_8,     1,  1.0f,  1.0f, "Gunpower Charlie" },
	{ "dacc0a82e8ee0c086971f9d9bac14127", CART_5200_EE_16, 1,  1.0f,  1.0f, "Gyruss (USA)" },
	{ "9983ab618fac2469f82bac1fc19f895b", CART_5200_EE_16, 1,  1.0f,  1.0f, "Gyruss (USA) (Alt)" },
	{ "b7617ac90462ef13f8350e32b8198873", CART_5200_EE_16, 1,  1.0f,  1.0f, "Gyruss (Autofire Hack)" },
	{ "f8f0e0a6dc2ffee41b2a2dd736cba4cd", CART_5200_NS_16, 1,  1.0f,  1.0f, "H.E.R.O. (USA)" },
	{ "3491fa368ae42766a83a43a627496c41", CART_5200_EE_16, 1,  1.0f,  1.0f, "Hangly Pollux" },
	{ "0f6407d83115a78a182f323e5ef76384", CART_5200_NS_16, 1,  1.0f,  1.0f, "Heavy Metal" },
	{ "0c25803c9175487afce0c9d636133dc1", CART_5200_32,    1,  1.0f,  1.0f, "Hyperblast! (XL Conversion)" },
	{ "d824f6ee24f8bc412468268395a76159", CART_5200_32,    1,  1.0f,  1.0f, "Ixion (XL Conversion)" },
	{ "936db7c08e6b4b902c585a529cb15fc5", CART_5200_EE_16, 1,  1.0f,  1.0f, "James Bond 007 (USA)" },
	{ "082846d3a43aab4672fe98252eb1b6f9", CART_5200_32,    1,  1.0f,  1.0f, "Jawbreaker (XL Conversion)" },
	{ "32ad87f1f480a29caa2cef9f51129a5d", CART_5200_32,    1,  1.0f,  1.0f, "Jetboot Jack (Conversion)" },
	{ "7a32eb8e820f5aaab755637745d36f01", CART_5200_32,    1,  1.0f,  1.0f, "Journey To The Planets (Conversion)" },
	{ "25cfdef5bf9b126166d5394ae74a32e7", CART_5200_EE_16, 1,  1.0f,  1.0f, "Joust (USA)" },
	{ "bc748804f35728e98847da6cdaf241a7", CART_5200_EE_16, 1,  1.0f,  1.0f, "Jr. Pac-Man (USA) (Proto)" },
	{ "40f3fca978058da46cd3e63ea8d2412f", CART_5200_EE_16, 1,  1.0f,  1.0f, "Jr Pac-Man (1984) (Atari) (U)" },
	{ "a0d407ab5f0c63e1e17604682894d1a9", CART_5200_32,    1,  1.0f,  1.0f, "Jumpman Jr (Conv)" },
	{ "27140302a715694401319568a83971a1", CART_5200_32,    1,  1.0f,  1.0f, "Jumpman Jr (XL Conversion)" },
	{ "1a6ccf1152d2bcebd16f0989b8257108", CART_5200_32,    1,  1.0f,  1.0f, "Jumpman Jr (XL Conversion)" },
	{ "834067fdce5d09b86741e41e7e491d6c", CART_5200_EE_16, 1,  1.0f,  1.0f, "Jungle Hunt (USA)" },
	{ "9584d143be1871241e4a0d038e8e1468", CART_5200_32,    1,  1.0f,  1.0f, "Juno First (XL Conversion)" },
	{ "92fd2f43bc0adf2f704666b5244fadf1", CART_5200_4,     1,  1.0f,  1.0f, "Kaboom! (USA)" },
	{ "796d2c22f8205fb0ce8f1ee67c8eb2ca", CART_5200_EE_16, 1,  1.0f,  1.0f, "Kangaroo (USA)" },
	{ "22097e3b37e4f1b68e86ad3787575cab", CART_5200_EE_16, 1,  1.0f,  1.0f, "Kangaroo (US)[b2]" },
	{ "f25a084754ea4d37c2fb1dc8ca6dc51b", CART_5200_8,     1,  1.0f,  1.0f, "Keystone Kapers (USA)" },
	{ "3b03e3cda8e8aa3beed4c9617010b010", CART_5200_32,    1,  1.0f,  1.0f, "Koffi - Yellow Kopter (USA) (Unl)" },
	{ "03d0d59c5382b0a34a158e74e9bfce58", CART_5200_8,     1,  1.0f,  1.0f, "Kid Grid" },
	{ "b99f405de8e7700619bcd18524ba0e0e", CART_5200_8,     1,  1.0f,  1.0f, "K-Razy Shoot-Out (USA)" },
	{ "66977296ff8c095b8cb755de3472b821", CART_5200_8,     1,  1.0f,  1.0f, "K-Razy Shoot-Out (1982) (CBS) [h1] (Two Port)" },
	{ "5154dc468c00e5a343f5a8843a14f8ce", CART_5200_32,    1,  1.0f,  1.0f, "K-Star Patrol (XL Conversion)" },
	{ "c4931be078e2b16dc45e9537ebce836b", CART_5200_32,    1,  1.0f,  1.0f, "Laser Gates (Conversion)" },
	{ "46264c86edf30666e28553bd08369b83", CART_5200_NS_16, 1,  1.0f,  1.0f, "Last Starfighter, The (USA) (Proto)" },
	{ "d0a1654625dbdf3c6b8480c1ed17137f", CART_5200_EE_16, 1,  1.0f,  1.0f, "Looney Tunes Hotel (1983) (Atari) (Prototype)" },
	{ "ff785ce12ad6f4ca67f662598025c367", CART_5200_8,     1,  1.0f,  1.0f, "Megamania (1983) (Activision)" },
	{ "8311263811e366bf5ef07977d0f5a5ae", CART_5200_32,    1,  1.0f,  1.0f, "MajorBlink_5200_V2 (XL Conversion)" },
	{ "d00dff571bfa57c7ff7880c3ce03b178", CART_5200_32,    1,  1.0f,  1.0f, "Mario Brothers (1983) (Atari)" },
	{ "1cd67468d123219201702eadaffd0275", CART_5200_NS_16, 1,  1.0f,  1.0f, "Meteorites (USA)" },
	{ "bc33c07415b42646cc813845b979d85a", CART_5200_32,    1,  1.0f,  1.0f, "Meebzork (1983) (Atari)" },
	{ "24348dd9287f54574ccc40ee40d24a86", CART_5200_EE_16, 1,  1.0f,  1.0f, "Microgammon SB (1983) (Atari) (Prototype)" },
	{ "84d88bcdeffee1ab880a5575c6aca45e", CART_5200_NS_16, 0,  1.0f,  1.0f, "Millipede (USA) (Proto)" },
	{ "d859bff796625e980db1840f15dec4b5", CART_5200_NS_16, 1,  1.0f,  1.0f, "Miner 2049er Starring Bounty Bob (USA)" },
	{ "69d472a79f404e49ad2278df3c8a266e", CART_5200_EE_16, 0,  1.0f,  1.0f, "Miniature Golf (1983) (Atari)" },
	{ "972b6c0dbf5501cacfdc6665e86a796c", CART_5200_8,     1,  0.1f, 0.15f, "Missile Command (USA)" },
	{ "3090673bd3f8c04a92e391bf5540b88b", CART_5200_32,    1,  1.0f,  1.0f, "MC+final" },
	{ "694897cc0d98fcf2f59eef788881f67d", CART_5200_EE_16, 1,  1.0f,  1.0f, "Montezuma's Revenge featuring Panama Joe (USA)" },
	{ "296e5a3a9efd4f89531e9cf0259c903d", CART_5200_NS_16, 1,  1.0f,  1.0f, "Moon Patrol (USA)" },
	{ "2d8e6aa095bf2aee75406ade8b035a50", CART_5200_NS_16, 1,  1.0f,  1.0f, "Moon Patrol Sprite Hack (USA)" },
	{ "627dbb2f84daef11229a165a69d84e09", CART_5200_32,    1,  1.0f,  1.0f, "Moon Patrol Redux" },
	{ "618e3eb7ae2810768e1aefed1bfdcec4", CART_5200_8,     1,  1.0f,  1.0f, "Mountain King (USA)" },
	{ "23296829e0e1316541aa6b5540b9ba2e", CART_5200_8,     1,  1.0f,  1.0f, "Mountain King (1984) (Sunrise Software) [h1] (Two Port)" },
	{ "fc3ab610323cc34e7984f4bd599b871f", CART_5200_32,    1,  1.0f,  1.0f, "Mr Cool (XL Conversion)" },
	{ "d1873645fee21e84b25dc5e939d93e9b", CART_5200_8,     1,  1.0f,  1.0f, "Mr. Do!'s Castle (USA)" },
	{ "ef9a920ffdf592546499738ee911fc1e", CART_5200_EE_16, 1,  1.0f,  1.0f, "Ms. Pac-Man (USA)" },
	{ "8341c9a660280292664bcaccd1bc5279", CART_5200_32,    1,  1.0f,  1.0f, "Necromancer" },
	{ "2624f5747d3a80a734d5c015cd76b3f9", CART_5200_32,    1,  1.0f,  1.0f, "Necromancer (Alternate)" },
	{ "6c661ed6f14d635482f1d35c5249c788", CART_5200_32,    1,  1.0f,  1.0f, "Oils Well (XL Conversion)" },
	{ "5781071d4e3760dd7cd46e1061a32046", CART_5200_32,    1,  1.0f,  1.0f, "O'Riley's Mine (XL Conversion)" },
	{ "ba9c81ec09505f76ecabde9716f0e7c3", CART_5200_32,    1,  1.0f,  1.0f, "Pacman Arcade Demo V2" },
	{ "f1a4d62d9ba965335fa13354a6264623", CART_5200_EE_16, 1,  1.0f,  1.0f, "Pac-Man (USA)" },
	{ "e24490c20bf79c933e50c11a89018960", CART_5200_32,    1,  1.0f,  1.0f, "Pac-Man (Fixed Munch V2)" },
	{ "3e90be559f2baee9aceee5853141f284", CART_5200_32,    1,  1.0f,  1.0f, "Pac-Man Plus" },
	{ "43e9af8d8c648515de46b9f4bcd024d7", CART_5200_32,    1,  1.0f,  1.0f, "Pacific Coast Hwy (XL Conversion)" },
	{ "57c5b010ec9b5f6313e691bdda94e185", CART_5200_32,    0,  1.0f,  1.0f, "Pastfinder (XL Conversion)" },
	{ "a301a449fc20ad345b04932d3ca3ef54", CART_5200_32,    1,  0.5f,  0.5f, "Pengo (USA)" },
	{ "c4a14a88a4257970223b1ef9bf95da5b", CART_5200_NS_16, 0,  1.0f,  1.0f, "Phobos (not working)" },
	{ "b9e727eaef3463d5979ec06fc5bd5048", CART_5200_NS_16, 1,  1.0f,  1.0f, "Pinhead" },
	{ "ecbd6dd2ab105dd43f98476966bbf26c", CART_5200_8,     1,  1.0f,  1.0f, "Pitfall! (USA) (use classics fix instead)" },
	{ "2be3529c33fdf6b76fa7528ba43cdd7f", CART_5200_32,    1,  1.0f,  1.0f, "Pitfall (classics fix)" },
	{ "e600c16c2b1f063ffb3f96caf4d23235", CART_5200_32,    1,  1.0f,  1.0f, "Pitstop (XL Conversion)" },
	{ "9e296c2817cbe1671005cf4dfebe8721", CART_5200_32,    1,  1.0f,  1.0f, "Protector II (XL Conversion)" },
	{ "fd0cbea6ad18194be0538844e3d7fdc9", CART_5200_EE_16, 1, 0.25f,  0.4f, "Pole Position (USA)" },
	{ "c3fc21b6fa55c0473b8347d0e2d2bee0", CART_5200_32,    1,  1.0f,  1.0f, "Pooyan" },
	{ "dd4ae6add63452aafe7d4fa752cd78ca", CART_5200_EE_16, 1,  1.0f,  1.0f, "Popeye (USA)" },
	{ "66057fd4b37be2a45bd8c8e6aa12498d", CART_5200_32,    1,  1.0f,  1.0f, "Popeye Arcade Final (Hack)" },
	{ "894959d9c5a88c8e1744f7fcbb930065", CART_5200_32,    1,  1.0f,  1.0f, "Preppie! (XL Conversion)" },
	{ "03ce205f9f1796183005301be5ae87fc", CART_5200_NS_16, 1,  1.0f,  1.0f, "Preppie! II (Conversion)" },
	{ "9e296c2817cbe1671005cf4dfebe8721", CART_5200_32,    1,  1.0f,  1.0f, "Protector II (Conversion)" },
	{ "ccd35e9ea3b3c5824214d88a6d8d8f7e", CART_5200_8,     1,  1.0f,  1.0f, "Pete's Diagnostics (1982) (Atari)" },
	{ "7830f985faa701bdec47a023b5953cfe", CART_5200_32,    0,  1.0f,  1.0f, "Emi Pool (XL Conversion)" },
	{ "ce44d14341fcc5e7e4fb7a04f77ffec9", CART_5200_8,     1,  1.0f,  1.0f, "Q-bert (USA)" },
	{ "9b7d9d874a93332582f34d1420e0f574", CART_5200_EE_16, 1,  1.0f,  1.0f, "QIX (USA)" },
	{ "099706cedd068aced7313ffa371d7ec3", CART_5200_NS_16, 0,  1.0f,  1.0f, "Quest for Quintana Roo (USA)" },
	{ "80e0ad043da9a7564fec75c1346dbc6e", CART_5200_NS_16, 1,  1.0f,  1.0f, "Rainbow Walker (Conversion)" },
	{ "150ff18392c270001f10e7934b2af546", CART_5200_32,    1,  1.0f,  1.0f, "Rally Speedway (XL Conversion)" },
	{ "a157be3500786000e138711c4826b16c", CART_5200_32,    1,  1.0f,  1.0f, "Raster Music Tracker" },
	{ "88fa71fc34e81e616bdffc30e013330b", CART_5200_32,    1,  1.0f,  1.0f, "Ratcatcher" },
	{ "623af6c0723c9c6f2707f247defbfa2f", CART_5200_32,    1,  1.0f,  1.0f, "Ratcatcher (Alternate)" },
	{ "2bb928d7516e451c6b0159ac413407de", CART_5200_32,    1,  1.0f,  1.0f, "RealSports Baseball (USA)" },
	{ "e056001d304db597bdd21b2968fcc3e6", CART_5200_32,    1,  1.0f,  1.0f, "RealSports Basketball (USA)" },
	{ "022c47b525b058796841134bb5c75a18", CART_5200_EE_16, 1,  1.0f,  1.0f, "RealSports Football (USA)" },
	{ "3074fad290298d56c67f82e8588c5a8b", CART_5200_EE_16, 1,  1.0f,  1.0f, "RealSports Soccer (USA)" },
	{ "3832950df7b057b02bd4c83642a75d17", CART_5200_EE_16, 1,  1.0f,  1.0f, "RealSports Soccer (US)[a]" },
	{ "7e683e571cbe7c77f76a1648f906b932", CART_5200_EE_16, 1,  1.0f,  1.0f, "RealSports Tennis (USA)" },
	{ "0dc44c5bf0829649b7fec37cb0a8186b", CART_5200_32,    1,  1.0f,  1.0f, "Rescue on Fractalus! (USA)" },
	{ "ddf7834a420f1eaae20a7a6255f80a99", CART_5200_EE_16, 1,  1.0f,  1.0f, "Road Runner (USA) (Proto)" },
	{ "86b358c9bca97c2089b929e3b2751908", CART_5200_32,    1,  1.0f,  1.0f, "Rockball 5200" },
	{ "09041900aa29078bcd1eb9c83cd15549", CART_5200_32,    1,  1.0f,  1.0f, "Rolltris" },
	{ "5dba5b478b7da9fd2c617e41fb5ccd31", CART_5200_NS_16, 0,  1.0f,  1.0f, "Robotron 2084 (USA)" },
	{ "b8cbc918cf2bc81f941719b874f13fcb", CART_5200_32,    1,  1.0f,  1.0f, "Runner5200" },
	{ "950aa1075eaf4ee2b2c2cfcf8f6c25b4", CART_5200_32,    1,  1.0f,  1.0f, "Satans Hollow (Conversion)" },
	{ "b610a576cbf26a259da4ec5e38c33f09", CART_5200_NS_16, 1,  1.0f,  1.0f, "Savage Pond (XL Conversion)" },
	{ "2892e8da83c840ce4165925b30b1f740", CART_5200_NS_16, 1,  1.0f,  1.0f, "Savage Pond (Alternate) (XL Conversion)" },
	{ "467e72c97db63eb59011dd062c965ec9", CART_5200_32,    1,  1.0f,  1.0f, "Scramble" },
	{ "3748e136c451471cdf58c94b251d925f", CART_5200_NS_16, 1,  1.0f,  1.0f, "Sea Chase" },
	{ "1aadd70705d84299085845989ec614ef", CART_5200_NS_16, 1,  1.0f,  1.0f, "Sea Dragon" },
	{ "54aa9130fa0a50ab8a74ed5b9076ff81", CART_5200_32,    1,  1.0f,  1.0f, "Shamus (XL Conversion)" },
	{ "37ec5b9d35ae681934698fea36e99aba", CART_5200_32,    1,  1.0f,  1.0f, "Shamus Case II (XL Conversion)" },
	{ "be75afc33f5da12974900317d824f9b9", CART_5200_32,    1,  1.0f,  1.0f, "Sinistar" },
	{ "6151575ffb5ceddd26173f709336776b", CART_5200_32,    1,  1.0f,  1.0f, "Slime (XL Conversion)" },
	{ "6e24e3519458c5cb95a7fd7711131f8d", CART_5200_EE_16, 1,  1.0f,  1.0f, "Space Dungeon (USA)" },
	{ "58430368d2c9190083f95ce923f4c996", CART_5200_8,     1,  1.0f,  1.0f, "Space Invaders (USA)" },
	{ "802a11dfcba6229cc2f93f0f3aaeb3aa", CART_5200_NS_16, 1,  1.0f,  1.0f, "Space Shuttle - A Journey Into Space (USA)" },
	{ "88d286e4b5fbbe7fd1694d98af9ef538", CART_5200_32,    1,  1.0f,  1.0f, "SpeedAce 5200" },
	{ "cd1c3f732c3432c4a642732182b1ea30", CART_5200_32,    1,  1.0f,  1.0f, "Spitfire (1984) (Atari) (Prototype)" },
	{ "221d943b1043f5bdf2b0f25282183404", CART_5200_32,    1,  1.0f,  1.0f, "Spitfire (Prototype)" },
	{ "993e3be7199ece5c3e03092e3b3c0d1d", CART_5200_EE_16, 1,  1.0f,  1.0f, "Sport Goofy (1983) (Atari) (Prototype)" },
	{ "6208110dc3c0bf7b15b33246f2971b6e", CART_5200_32,    1,  1.0f,  1.0f, "Spy Hunter (XL Conversion)" },
	{ "595703dc459cd51fed6e2a191c462969", CART_5200_EE_16, 1,  1.0f,  1.0f, "Stargate (1984) (Atari)" },
	{ "8378e0f92e9365a6ad42efc9b973724a", CART_5200_NS_16, 1,  1.0f,  1.0f, "Star Island" },
	{ "e2d3a3e52bb4e3f7e489acd9974d68e2", CART_5200_EE_16, 0,  0.7f,  1.0f, "Star Raiders (USA)" },
	{ "64579ed5a28e3174262aa5283e41b66a", CART_5200_EE_16, 0,  0.7f,  1.0f, "Star Raiders (USA) (Alternate)" },
	{ "0fe34d98a055312aba9ea3cb82d3ee2a", CART_5200_32,    0,  1.0f,  1.0f, "Star Raiders 5200(shield2-02)(32K)" },
	{ "feacc7a44f9e92d245b2cb2485b48bb6", CART_5200_NS_16, 1,  1.0f,  1.0f, "Star Rider" },
	{ "c959b65be720a03b5479650a3af5a511", CART_5200_EE_16, 1,  1.0f,  1.0f, "Star Trek - Strategic Operations Simulator (USA)" },
	{ "00beaa8405c7fb90d86be5bb1b01ea66", CART_5200_EE_16, 1,  1.0f,  1.0f, "Star Wars - The Arcade Game (USA)" },
	{ "a2831487ab0b0b647aa590fb2b834dd9", CART_5200_8,     1,  1.0f,  1.0f, "Star Wars - ROTJ - Death Star Battle (1983) (Parker Bros)" },
	{ "865570ff9052c1704f673e6222192336", CART_5200_4,     1, 0.12f, 0.25f, "Super Breakout (USA)" },
	{ "dfcd77aec94b532728c3d1fef1da9d85", CART_5200_8,     1,  1.0f,  1.0f, "Super Cobra (USA)" },
	{ "d89669f026c34de7f0da2bcb75356e27", CART_5200_EE_16, 1,  1.0f,  1.0f, "Super Pac Man Final (5200)" },
	{ "1569b7869bf9e46abd2c991c3b90caa6", CART_5200_NS_16, 1,  1.0f,  1.0f, "Superfly (XL Conversion)" },
	{ "c098a0ce6c7e059264511e650ce47b35", CART_5200_32,    1,  1.0f,  1.0f, "Tapper (XL Conversion)" },
	{ "59699274930349b4fe0ee94b21ce3c16", CART_5200_32,    1,  1.0f,  1.0f, "Tapper (Alternate) (XL Conversion)" },
	{ "496b6a002bc7d749c02014f7ec6c303c", CART_5200_NS_16, 1,  1.0f,  1.0f, "Tempest (1983) (Atari) (Prototype) [!]" },
	{ "5624c42cdbbffa6ec5b0f97252ce696e", CART_5200_32,    1,  1.0f,  1.0f, "Thetris" },
	{ "6836a07ea7b2a4c071e9e86c5695b4a1", CART_5200_32,    1,  1.0f,  1.0f, "Timeslip_5200 (XL Conversion)" },
	{ "bb3761de48d39218744d7dbb94553528", CART_5200_NS_16, 1,  1.0f,  1.0f, "Time Runner (XL Conversion)" },
	{ "3f4d175927f891642e5c9f8a197c7d89", CART_5200_32,    1,  1.0f,  1.0f, "Time Runner 32k (BIOS Patched)" },
	{ "bf4f25d64b364dd53fbd63562ea1bcda", CART_5200_32,    1,  1.0f,  1.0f, "Turmoil (XL Conversion)" },
	{ "ae76668cf509a13872ccd874ac47206b", CART_5200_32,    1,  1.0f,  1.0f, "Tutankahman" },
	{ "3649bfd2008161b9825f386dbaff88da", CART_5200_32,    0,  1.0f,  1.0f, "Up'n Down (XL Conversion)" },
	{ "556a66d6737f0f793821e702547bc051", CART_5200_32,    1,  1.0f,  1.0f, "Vanguard (USA)" },
	{ "560b68b7f83077444a57ebe9f932905a", CART_5200_NS_16, 1,  1.0f,  1.0f, "Wizard of Wor (USA)" },
	{ "8e2ac7b944c30af9fae5f10c3a40f7a4", CART_5200_32,    1,  1.0f,  1.0f, "Worm War I (XL Conversion)" },
	{ "677e4fd5bba70f5983d2c2bbfba36b7e", CART_5200_32,    0,  1.0f,  1.0f, "Xagon (XL Conversion)" },
	{ "4f6c58c28c41f31e3a1515fe1e5d15af", CART_5200_EE_16, 1,  1.0f,  1.0f, "Xari Arena (USA) (Proto)" },
	{ "f35f9e5699079e2634c4bfed0c5ef2f0", CART_5200_8,     1,  1.0f,  1.0f, "Yars Strike (XL Conversion)" },
	{ "9fee054e7d4ba2392f4ba0cb73fc99a5", CART_5200_32,    1,  1.0f,  1.0f, "Zaxxon (USA)" },
	{ "94f17ee9875c7c32f3c3673276c5d46d", CART_5200_32,    1,  1.0f,  1.0f, "Zaxxon 32k" },
	{ "433d3a2fc9896aa8294271a0204dc7e3", CART_5200_32,    1,  1.0f,  1.0f, "Zaxxon 32k_final" },
	{ "77beee345b4647563e20fd896231bd47", CART_5200_8,     1,  1.0f,  1.0f, "Zenji (USA)" },
	{ "dc45af8b0996cb6a94188b0be3be2e17", CART_5200_NS_16, 1,  1.0f,  1.0f, "Zone Ranger (USA)" },
	{ "322cf3f7ff6515e2f363927134146b59", CART_5200_64,    1,  1.0f,  1.0f, "Berks 4" },
	{ "cea3ea765d3626be01ed9b290c9a9bd0", CART_5200_512,   1,  1.0f,  1.0f, "Bosconian" },
	{ "c4ea4997cf906dd20ae474eebe1d2a04", CART_5200_64,    1,  1.0f,  1.0f, "Dropzone" },
	{ "b5cd13d78d7124b34f5c77d7f8739bb4", CART_5200_8,     1,  1.0f,  1.0f, "Enduro" },
	{ "4e16903c352c8ed75ed9377e72ebe333", CART_5200_64,    1,  1.0f,  1.0f, "Laser Hawk" },
	{ "4aeda426bce595851f3edd0c7d3a05fc", CART_5200_64,    1,  1.0f,  1.0f, "Mr. Do" },
	{ "a6ed56ea679e6279d0baca2e5cafab78", CART_5200_64,    1,  1.0f,  1.0f, "M.U.L.E" },
	{ "0ff19bfcf911b385851eb7af07cb5034", CART_5200_64,    1,  1.0f,  1.0f, "Oil's Well (64k)" },
	{ "cdf2e6b37e3480af922159a7fc13ac43", CART_5200_64,    1,  1.0f,  1.0f, "Rampage (64k)" },
	{ "bd4bb4dd468601a2241233778f328267", CART_5200_64,    1,  1.0f,  1.0f, "Sea Dragon (64k)" },
	{ "122706075c94269faeea7c252a20da61", CART_5200_64,    1,  1.0f,  1.0f, "Super Pac-Man (64k)" },
	{ "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", CART_NONE,       1,  1.0f,  1.0f, "NONE" },
};

/* For cartridge memory */
const UBYTE *cart_image = NULL;
int super_cart_bank = 1;
int super_cart_size = 0;

struct cart_info_t cart_info = {0};

/* a read from D500-D5FF area */
UBYTE CART_GetByte(UWORD addr)
{
	if (rtime8_enabled && (addr == 0xd5b8 || addr == 0xd5b9))
		return RTIME8_GetByte();
	return 0xff;
}

/* a write to D500-D5FF area */
void CART_PutByte(UWORD addr, UBYTE byte)
{
	if (rtime8_enabled && (addr == 0xd5b8 || addr == 0xd5b9)) {
		RTIME8_PutByte(byte);
		return;
	}
}

/* special support of Bounty Bob on Atari5200 */
void CART_BountyBob1(UWORD addr)
{
	if (addr >= 0x4ff6 && addr <= 0x4ff9) {
		addr -= 0x4ff6;
		if (cart_info.type == CART_5200_40_ALT)
			CopyROM(0x4000, 0x4fff, cart_image + 0x2000 + addr * 0x1000);
		else
			CopyROM(0x4000, 0x4fff, cart_image + addr * 0x1000);
	}
}

void CART_BountyBob2(UWORD addr)
{
	if (addr >= 0x5ff6 && addr <= 0x5ff9) {
		addr -= 0x5ff6;
		if (cart_info.type == CART_5200_40_ALT)
			CopyROM(0x5000, 0x5fff, cart_image + 0x6000 + addr * 0x1000);
		else
			CopyROM(0x5000, 0x5fff, cart_image + 0x4000 + addr * 0x1000);
	}
}

static void set_bank_5200_SUPER(void)
{
	CopyROM(0x4000, 0xbfff, cart_image + super_cart_bank * 0x8000);
}

/* addr must be $bfxx in 5200 mode only. */
static void access_5200SuperCart(UWORD addr)
{
	int old_bank = super_cart_bank;
	int new_bank = old_bank;

	if ((addr & 0xc0) == 0xc0) {
		switch (addr & 0x30) {
		case 0x00: /* $BFCx */
			new_bank = (new_bank & 0x03) | (addr & 0x0c);
			break;
		case 0x10: /* $BFDx */
			new_bank = (new_bank & 0x0c) | ((addr & 0x0c) >> 2);
			break;
		default: /* 0x20 or 0x30, i.e. $BFEx or $BFFx */
			new_bank = 0x0f;
			break;
		}
		new_bank &= ((super_cart_size >> 5) - 1);
	}

	if (old_bank != new_bank) {
		super_cart_bank = new_bank;
		set_bank_5200_SUPER();
	}
}

UBYTE CART_5200SuperCartGetByte(UWORD addr)
{
	access_5200SuperCart(addr);
	return dGetByte(addr);
}

void CART_5200SuperCartPutByte(UWORD addr, UBYTE value)
{
	access_5200SuperCart(addr);
}

int CART_Insert(const uint8_t *data, size_t size)
{
	const struct cart_info_t *cart_info_entry = NULL;
	unsigned char md5_digest[16] = {0};
	char md5_hash[33] = {0};
	MD5_CTX md5_ctx;
	size_t size_kb;

	/* remove currently inserted cart */
	CART_Remove();

	if (data == NULL || size < 16)
		return CART_CANT_OPEN;

	/* Assign pointer */
	cart_image = (UBYTE *)data;
	super_cart_bank = 1;

	/* find cart type */
	memcpy(&cart_info, &cart_info_none, sizeof(cart_info));

	size_kb = size >> 10; /* number of kilobytes */
	super_cart_size = size_kb;

	if      (size_kb == 4)  cart_info.type = CART_5200_4;
	else if (size_kb == 8)  cart_info.type = CART_5200_8;
	else if (size_kb == 16) cart_info.type = CART_5200_NS_16;
	else if (size_kb == 32) cart_info.type = CART_5200_32;
	else if (size_kb == 40) cart_info.type = CART_5200_40;
	else if (size_kb == 64) cart_info.type = CART_5200_64;
	else if (size_kb == 128) cart_info.type = CART_5200_128;
	else if (size_kb == 256) cart_info.type = CART_5200_256;
	else if (size_kb == 5120) cart_info.type = CART_5200_512;


	/* Get md5sum of cart data and check if it matches
	 * an entry in the master list */
	MD5_Init(&md5_ctx);
	MD5_Update(&md5_ctx, data, size);
	MD5_Final(md5_digest, &md5_ctx);

	snprintf(md5_hash, sizeof(md5_hash),
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			md5_digest[0],  md5_digest[1],  md5_digest[2],  md5_digest[3],
			md5_digest[4],  md5_digest[5],  md5_digest[6],  md5_digest[7],
			md5_digest[8],  md5_digest[9],  md5_digest[10], md5_digest[11],
			md5_digest[12], md5_digest[13], md5_digest[14], md5_digest[15]);

	for (cart_info_entry = cart_info_table;
		  cart_info_entry->type != CART_NONE;
		  cart_info_entry++)
		if (string_is_equal(md5_hash, cart_info_entry->md5))
		{
			memcpy(&cart_info, cart_info_entry, sizeof(cart_info));
			a5200_log(RETRO_LOG_INFO, "Detected cart: %s type: %d\n", cart_info.name, cart_info.type);
			break;
		}

	/* If valid cart is detected, start it */
	if (cart_info.type != CART_NONE)
	{
		CART_Start();
		return 0;
	}

	cart_image = NULL;
	return CART_BAD_FORMAT;
}

void CART_Remove(void) {
	memcpy(&cart_info, &cart_info_none, sizeof(cart_info));
	cart_image = NULL;
	CART_Start();
}

void CART_Start(void) {
	SetROM(0x4ff6, 0x4ff9);		/* disable Bounty Bob bank switching */
	SetROM(0x5ff6, 0x5ff9);
	SetROM(0xbfc0, 0xbfff); /* disable Super Cart bank switching */
	switch (cart_info.type) {
	case CART_5200_64:
	case CART_5200_128:
	case CART_5200_256:
	case CART_5200_512:
		set_bank_5200_SUPER();
		SetHARDWARE(0xbfc0, 0xbfff);
		break;
	case CART_5200_32:
		CopyROM(0x4000, 0xbfff, cart_image);
		break;
	case CART_5200_EE_16:
		CopyROM(0x4000, 0x5fff, cart_image);
		CopyROM(0x6000, 0x9fff, cart_image);
		CopyROM(0xa000, 0xbfff, cart_image + 0x2000);
		break;
	case CART_5200_40:
		CopyROM(0x4000, 0x4fff, cart_image);
		CopyROM(0x5000, 0x5fff, cart_image + 0x4000);
		CopyROM(0x8000, 0x9fff, cart_image + 0x8000);
		CopyROM(0xa000, 0xbfff, cart_image + 0x8000);
		SetHARDWARE(0x4ff6, 0x4ff9);
		SetHARDWARE(0x5ff6, 0x5ff9);
		break;
	case CART_5200_40_ALT:
		CopyROM(0x4000, 0x4fff, cart_image + 0x2000);
		CopyROM(0x5000, 0x5fff, cart_image + 0x6000);
		CopyROM(0x8000, 0x9fff, cart_image);
		CopyROM(0xa000, 0xbfff, cart_image);
		SetHARDWARE(0x4ff6, 0x4ff9);
		SetHARDWARE(0x5ff6, 0x5ff9);
		break;
	case CART_5200_NS_16:
		CopyROM(0x8000, 0xbfff, cart_image);
		break;
	case CART_5200_8:
		CopyROM(0x8000, 0x9fff, cart_image);
		CopyROM(0xa000, 0xbfff, cart_image);
		break;
	case CART_5200_4:
		CopyROM(0x8000, 0x8fff, cart_image);
		CopyROM(0x9000, 0x9fff, cart_image);
		CopyROM(0xa000, 0xafff, cart_image);
		CopyROM(0xb000, 0xbfff, cart_image);
		break;
	default:
		/* clear cartridge area so the 5200 will crash */
		dFillMem(0x4000, 0, 0x8000);
		break;
	}
}

void CARTStateRead(void)
{
	/* Unused, but load the cart type for backwards
	 * compatibility with old state files */
	int savedCartType = CART_NONE;
	ReadINT(&savedCartType, 1);

	/* Need this to restore and set current bank for Super Carts*/
	switch (cart_info.type) {
	case CART_5200_64:
	case CART_5200_128:
	case CART_5200_256:
	case CART_5200_512:
		ReadINT(&super_cart_bank, 1);
		set_bank_5200_SUPER();
		break;
	}

}

void CARTStateSave(void)
{
	/* Unused, but save the cart type for backwards
	 * compatibility with old state files */
	SaveINT(&cart_info.type, 1);

	/* Need this to restore and set current bank for Super Carts*/
	switch (cart_info.type) {
	case CART_5200_64:
	case CART_5200_128:
	case CART_5200_256:
	case CART_5200_512:
		SaveINT(&super_cart_bank, 1);
		break;
	}
}
