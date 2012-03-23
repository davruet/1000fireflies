import toxi.geom.Vec2D;
import toxi.geom.Rect;
import toxi.geom.PointQuadtree;
import java.util.HashMap;
import toxi.color.TColor;
import processing.video.*;
import controlP5.*;

int count = 1000;
static int ON_THRESHOLD = 700;
static int ON_DURATION = 100;
static int PHASE_THRESHOLD = ON_THRESHOLD + ON_DURATION;
static int ADJUSTMENT_MAX = ON_THRESHOLD / 2;

int radius = 320  ;
int syncMultiple = 75;

float timefactor = 1.0;

ArrayList<Vec2D> cyclists = new ArrayList<Vec2D>(count);
long lastTimeAdded;
long loopBegin;
ControlP5 controlP5;
MovieMaker mm;

void setup(){
  
  size(1280, 720);
  controlP5 = new ControlP5(this);
  controlP5.addSlider("radius", 0, 500);
  controlP5.addSlider("syncMultiple", 0, 100);
  controlP5.addSlider("timefactor", 1.0, 5.0);
  controlP5.hide();
   //
   
}


void update(){
  
  if (millis() > lastTimeAdded + 300 && cyclists.size() < 300){
    Cyclist c = new Cyclist();
    c.set(random(width), random(height));
    cyclists.add(c);
    lastTimeAdded = millis();
  }
  
  for (int i = 0; i< cyclists.size(); i++){
    Cyclist c = (Cyclist)cyclists.get(i);  
    c.update();
    
    
  }
  
  
  PointQuadtree tree = new PointQuadtree(new Vec2D(-1,-1), 1280);
  tree.addAll(cyclists);
  for (int i = 0; i< cyclists.size(); i++){
    Cyclist c = (Cyclist)cyclists.get(i);
    if (c.isActive()){
      Rect rect = new Rect(c.x() - radius / 2, c.y - radius / 2, radius, radius);
      ArrayList<Vec2D> neighbors = tree.getPointsWithinRect(rect);
      if (neighbors != null){
        for (Vec2D neighborV : neighbors){
          Cyclist neighbor = (Cyclist)neighborV;
          neighbor.pulse();
        }
      }  
    }
  }
  
}

TColor light = TColor.newRGB(255,0,0);

void draw(){
  loopBegin = millis();
  update();
  background(0, 0, 25);
  
  for (int i = 0; i< cyclists.size(); i++){
    Cyclist c = (Cyclist)cyclists.get(i); 
    TColor col = TColor.newRGB(.1,.1,.2);
    if (c.isActive()){
       col.blend(light, c.getIntensity());
      fill(col.toARGB());
      if (c.getIntensity() > 1 || c.getIntensity() < 0) println(c.getIntensity());
      //fill(255);
    }
    else fill(col.toARGB());
    ellipse(c.x(), c.y(), 10, 10);
  }
  // throttle
  //delay((int)Math.max(0, 5 - (millis() - loopBegin)));
  if (mm != null) mm.addFrame();
}


class Cyclist extends Vec2D {
  float intensity = random(PHASE_THRESHOLD);  
  int phaseAdjustment = 0;
   
   
  Vec2D velocity = new Vec2D();
  
  void update(){
    /*velocity = velocity.add( (noise(position.x(), position.y(), millis()/10f) - .5) ,
                  (noise(position.x(), position.y(), millis()/10f) - .5));*/
                  
    velocity = velocity.jitter(.01);
    this.addSelf(velocity);
    if (this.x() < 0 || this.x() > width) {
      velocity = velocity.scale(-1,1);
    }
    if (this.y() < 0 || this.y() > height) {
      velocity = velocity.scale(1,-1);
    }
    
    velocity = velocity.scale(.999);
    
    
    intensity+= 10 * timefactor;
    if (intensity >= PHASE_THRESHOLD){
      intensity = 0;
      phaseAdjustment = 0;
    }
  }
  
  void pulse(){
    // if already on, don't listen for pulses, increment normally.
    if (isActive()) return;
    float increment = 0;
    if (intensity < ON_THRESHOLD / 2){
      increment = -1 * Math.max(0, intensity) / syncMultiple;     
    } else {
      increment = ON_THRESHOLD - Math.min(ON_THRESHOLD, intensity) / syncMultiple;
    }
    phaseAdjustment += increment * timefactor;
    if (phaseAdjustment > -ADJUSTMENT_MAX && phaseAdjustment < ADJUSTMENT_MAX){
      intensity += increment;  
    }
    
  }
  
  boolean isActive(){
    return intensity > ON_THRESHOLD; 
  }
  
  float getIntensity(){
    
    return Math.max(0, ((float)ON_DURATION - (intensity - ON_THRESHOLD)) / ON_DURATION); 
  }
  
}
int movieId = 0;

void keyPressed(){
  switch (key){
     case ' ':
      if (controlP5.isVisible()) controlP5.hide();
      else controlP5.show();
      break;
     case 'r':
       if (mm == null){
         mm = new MovieMaker(this, width, height, "cycling" + movieId++ + ".mov",
                       30, MovieMaker.ANIMATION, MovieMaker.BEST);
         timefactor = 2;
       } else {
         mm.finish();
         mm = null;
         timefactor = 1;
       }
                       
  }
 // mm.finish();  
}

