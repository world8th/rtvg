//#[macro_use]
extern crate ash;

//#[macro_use]
extern crate examples;
use examples::*;

fn main(){
    let renderer: Renderer = Renderer::new();
    renderer.render_loop(&960u32,&540u32);
}
