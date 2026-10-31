#pragma once
namespace Port { namespace AuthorityBootstrap {
// Called by game_main after the ordinary resource/display/executive startup,
// for --authority-restore. JS provides authenticated room metadata in
// window.homm3AuthorityRestore={map,setup:[41 i32],actor}. No save or RNG state.
void run();
} }
