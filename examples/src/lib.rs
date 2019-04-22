#[macro_use]
extern crate ash;
#[cfg(target_os = "windows")]
extern crate winapi;

#[cfg(target_os = "macos")]
extern crate cocoa;
#[cfg(target_os = "macos")]
extern crate metal_rs as metal;
#[cfg(target_os = "macos")]
extern crate objc;
extern crate winit;

#[macro_use]
pub mod modules;
pub use modules::*;
