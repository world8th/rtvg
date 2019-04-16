//extern crate ash;
//extern crate examples;

pub mod display;
use display::*;

fn main(){
    let renderer: Renderer = Renderer::new();
    renderer.render_loop(&1920u32,&1080u32);
}
