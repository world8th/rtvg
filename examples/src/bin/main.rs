extern crate ash;
extern crate examples;

pub mod display;

fn main(){
    let renderer: display::Renderer = display::Renderer::new();
    renderer.render_loop(&1920u32,&1080u32);
}
