//#[macro_use]
extern crate ash;

//#[macro_use]
extern crate examples;
use examples::*;

fn main(){
    let renderer: Renderer = Renderer::new(&960u32,&540u32);



    
    renderer.render_loop();
}
