/*
 * Copyright (c) 2013 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxMidi for documentation
 *
 */
#include "ofApp.h"
#include "iostream"
#define MIDI_MAGIC 63.50f
#define CONTROL_THRESHOLD .025f

//dummy variables for midi to audio attenuatiors
//0 is direct midi, 1 is low_x, 2 is mid_x, 3 is high_x
int control_switch=0;

//put these into arrays
const int controlSize=17;
float fftLow[controlSize];
float fftMid[controlSize];
float fftHigh[controlSize];

float low_value_smoothed=0.0;
float mid_value_smoothed=0.0;
float high_value_smoothed=0.0;
float smoothing_rate=.8;

float fft_low=0;
float fft_mid=0;
float fft_high=0;
//the range changes depending on what yr using for sound input.  just usb cams have shitter ranges
//so 50 works
float range=75;

bool clear_switch=0;


//p_lock biz
//maximum total size of the plock array
const int p_lock_size=240;
bool p_lock_record_switch=0;
bool p_lock_erase=0;
//maximum number of p_locks available...maybe there can be one for every knob
//for whatever wacky reason the last member of this array of arrays has a glitch
//so i guess just make an extra array and forget about it for now
const int p_lock_number=17;
//so how we will organize the p_locks is in multidimensional arrays
//to access the data at timestep x for p_lock 2 (remember counting from 0) we use p_lock[2][x]
float p_lock[p_lock_number][p_lock_size];
//smoothing parameters(i think for all of these we can array both the arrays and the floats
//for now let us just try 1 smoothing parameter for everything.
float p_lock_smooth=.5;
//and then lets try an array of floats for storing the smoothed values
float p_lockSmoothed[p_lock_number];
//turn on and off writing to the array
bool p_lock_0_switch=1;
//global counter for all the locks
int p_lock_increment=0;

float my_normalize=0;

//0 is picapture, 1 is usbinput
//this is only relevant for folks with the now discontinued capture editions
//or their own picapture sd1s.  most people can just ignore this switch!
bool inputswitch=1;

int hdmi_aspect_ratio_switch=0;

float rescale=1;

float theta=0;

float x_lfo=0;
float x_lfo_arg=0;
float y_lfo=0;
float y_lfo_arg=0;
float z_lfo=0;
float z_lfo_arg=0;

int width=640;
int height=480;

bool wireframe_switch=0;
bool bright_switch=0;

bool invert_switch=0;
bool stroke_weight_switch=0;

int scale=100;

int scale_key=0;

int yLfoShape=0;
int xLfoShape=0;
int zLfoShape=0;

bool y_freq0=FALSE;
bool y_ringmod_switch=FALSE;
bool y_phasemod_switch=FALSE;

bool x_freq0=FALSE;
bool x_ringmod_switch=FALSE;
bool x_phasemod_switch=FALSE;

bool z_freq0=FALSE;
bool z_ringmod_switch=FALSE;
bool z_phasemod_switch=FALSE;

float global_x_displace=0.0;
float global_y_displace=0.0;

float center_x_displace=0.0;
float rotate_x=0.0;
float rotate_capture_z=0.0;

bool rotate_capture_z_switch=1;
bool rotate_x_switch=1;
bool global_x_displace_switch=1;
bool center_x_displace_switch=0;


float center_y_displace=0.0;
float rotate_y=0.0;

bool rotate_y_switch=1;
bool global_y_displace_switch=1;
bool center_y_displace_switch=0;
bool luma_switch=0;

bool b_w_switch=0;

float stroke_weight=1;

//mesh types
//0 is trianglegrid
//1 is horizontal lines
//2 is vertical lines
int mesh_type=0;

//midi latching
bool midiActiveFloat[17];
bool lowActiveFloat[17];
bool midActiveFloat[17];
bool highActiveFloat[17];
//--------------------------------------------------------------
void ofApp::setup() {
    //ofSetVerticalSync(true);
    ofSetFrameRate(30);
    ofBackground(0);
    //ofToggleFullscreen();
    ofHideCursor();
    
    //shader_mixer.load("shadersES2/shader_mixer");
    midiSetup();
    
    allocateFbo();
    
    p_lockClear();
    
    shaderDisplace.load("shadersES2/shaderDisplace");
    trianglemesh(scale);
    //so 1/4 size doggg
    x_noise_image.allocate(180,120, OF_IMAGE_GRAYSCALE);
    y_noise_image.allocate(180,120, OF_IMAGE_GRAYSCALE);
    z_noise_image.allocate(180,120, OF_IMAGE_GRAYSCALE);
    
    //midi latching
    for(int i=0;i<17;i++){
        midiActiveFloat[i]=0;
        lowActiveFloat[i]=0;
        midActiveFloat[i]=0;
        highActiveFloat[i]=0;
    }
    
    //fft biz
    fft.setup();
    fft.setNormalize(false);
    //fft.setVolumeRange(1.0f);
    fftArrayClear();
    
    cameraSetup(640,480);
}

//--------------------------------------------------------------
void ofApp::update() {
    //this one points to the ofxFft library
    fft.update();
    //this one is local and made by me
    fftValuesUpdate();
    cameraUpdate();
    midibiz();
    p_lockUpdate();
    
    x_noise_image=perlin_noise(x_lfo_arg,p_lockSmoothed[4],x_noise_image);
    y_noise_image=perlin_noise(y_lfo_arg,p_lockSmoothed[5],y_noise_image);
    z_noise_image=perlin_noise(z_lfo_arg,p_lockSmoothed[3],z_noise_image);
}

//--------------------------------------------------------------
void ofApp::draw() {
    
    /**controlBIz**/
    float c_x=100.0;
    float c_y=100.0;
    float c_zFrequency=.03;
    float c_xFrequency=.015;
    float c_yFrequency=.02;
    float c_zoom=480.0f;
    
    float d_luma_key_level=p_lockSmoothed[0]+1.01*az;
    float d_x=c_x*(p_lockSmoothed[1]+qw);
    float d_y=c_y*(p_lockSmoothed[2]+er);
    float d_zFrequency=c_zFrequency*p_lockSmoothed[3]+sx;
    float d_xFrequency=c_xFrequency*p_lockSmoothed[4]+gb;
    float d_yFrequency=c_yFrequency*p_lockSmoothed[5]+kk;
    float d_zoom=c_zoom*p_lockSmoothed[6]+op;
    float d_scale=(1.0-p_lockSmoothed[7])*126.0+1.0+scale_key;
    
    float d_z_lfo_arg=p_lockSmoothed[10]+dc;
    float d_x_lfo_arg=p_lockSmoothed[12]+hn;
    float d_y_lfo_arg=p_lockSmoothed[14]+ll;
    float d_center_x=-960.0f*p_lockSmoothed[8]+ty;
    float d_center_y=-960.0f*p_lockSmoothed[9]+ui;
    float d_z_lfo_amp=.25*p_lockSmoothed[11]+fv;   
    float d_x_lfo_amp=ofGetWidth()*.25*p_lockSmoothed[13]+jm;   
    float d_y_lfo_amp=ofGetHeight()*.25*p_lockSmoothed[15]+ylfo_amp;
    
    //audio reactive biz
    d_luma_key_level+=fftLow[0]*low_value_smoothed+
    fftMid[0]*mid_value_smoothed+
    fftHigh[0]*high_value_smoothed;
    d_x+=c_x*(fftLow[1]*low_value_smoothed+
              fftMid[1]*mid_value_smoothed+
              fftHigh[1]*high_value_smoothed);
    d_y+=c_y*(fftLow[2]*low_value_smoothed+
              fftMid[2]*mid_value_smoothed+
              fftHigh[2]*high_value_smoothed);
    d_zFrequency+=c_zFrequency*(fftLow[3]*low_value_smoothed+
                                fftMid[3]*mid_value_smoothed+
                                fftHigh[3]*high_value_smoothed);						  				  
    d_xFrequency+=c_xFrequency*(fftLow[4]*low_value_smoothed+
                                fftMid[4]*mid_value_smoothed+
                                fftHigh[4]*high_value_smoothed);
    d_yFrequency+=c_yFrequency*(fftLow[5]*low_value_smoothed+
                                fftMid[5]*mid_value_smoothed+
                                fftHigh[5]*high_value_smoothed);
    d_zoom+=c_zoom*(fftLow[6]*low_value_smoothed+
                    fftMid[6]*mid_value_smoothed+
                    fftHigh[6]*high_value_smoothed);						  				  						  				  				  						  				  
    /**oscillator biz**/
    z_lfo_arg+=d_z_lfo_arg;
    x_lfo_arg+=d_x_lfo_arg;
    y_lfo_arg+=d_y_lfo_arg;
    
    /**shaderbiz***/
    fbo0.begin();
    shaderDisplace.begin();
    ofBackground(0);
    
    if(invert_switch==1){ofBackground(255);}
    
    shaderDisplace.setUniform1f("b_w_switch",b_w_switch);
    
    shaderDisplace.setUniformTexture("x_noise_image",x_noise_image.getTexture(),1);
    shaderDisplace.setUniformTexture("y_noise_image",y_noise_image.getTexture(),2);
    shaderDisplace.setUniformTexture("z_noise_image",z_noise_image.getTexture(),3);
    
    shaderDisplace.setUniform1f("luma_key_level",d_luma_key_level);
    
    ofVec2f xy;
    xy.set(d_x,d_y);
    shaderDisplace.setUniform2f("xy",xy);
    shaderDisplace.setUniform1i("width",width);
    shaderDisplace.setUniform1i("height",height);
    shaderDisplace.setUniform1i("bright_switch",bright_switch);
    shaderDisplace.setUniform1f("invert_switch",invert_switch);
    
    shaderDisplace.setUniform1f("z_lfo_amp",d_z_lfo_amp);
    shaderDisplace.setUniform1f("z_lfo_arg",z_lfo_arg);
    shaderDisplace.setUniform1f("z_lfo_other",d_zFrequency);
    
    shaderDisplace.setUniform1f("x_lfo_amp",d_x_lfo_amp);
    shaderDisplace.setUniform1f("x_lfo_arg",x_lfo_arg);
    shaderDisplace.setUniform1f("x_lfo_other", d_xFrequency);
    
    shaderDisplace.setUniform1f("y_lfo_amp",d_y_lfo_amp);
    shaderDisplace.setUniform1f("y_lfo_arg",y_lfo_arg);
    shaderDisplace.setUniform1f("y_lfo_other", d_yFrequency);
    
    ofVec2f xy_offset;
    xy_offset.set(d_center_x,d_center_y);
    shaderDisplace.setUniform2f("xy_offset",xy_offset);
    
    shaderDisplace.setUniform1i("yLfoShape",yLfoShape);
    shaderDisplace.setUniform1i("xLfoShape",xLfoShape);
    shaderDisplace.setUniform1i("zLfoShape",zLfoShape);
    
    shaderDisplace.setUniform1i("y_phasemod_switch",y_phasemod_switch);
    shaderDisplace.setUniform1i("x_phasemod_switch",x_phasemod_switch);
    shaderDisplace.setUniform1i("z_phasemod_switch",z_phasemod_switch);
    
    shaderDisplace.setUniform1i("y_ringmod_switch",y_ringmod_switch);
    shaderDisplace.setUniform1i("x_ringmod_switch",x_ringmod_switch);
    shaderDisplace.setUniform1i("z_ringmod_switch",z_ringmod_switch);
    
    shaderDisplace.setUniform1i("luma_switch",luma_switch);
    
    shaderDisplace.setUniform1i("width",width);
    shaderDisplace.setUniform1i("height",height);
    
    /**bind the vertices**/	
    if(inputswitch==0){
        //videoGrabber.getTextureReference().bind();
    }
    if(inputswitch==1){	
        if(hdmi_aspect_ratio_switch==0){
            cam1.getTexture().bind();
        }
        if(hdmi_aspect_ratio_switch==1){
            aspect_fix_fbo.getTexture().bind();
        }
    }
    
    glLineWidth(stroke_weight);
    
    /**make the vertices happen**/
    ofPushMatrix();
    ofTranslate(0,0,d_zoom);
    if(wireframe_switch==0){
        vbo_mesh1.draw();
    }
    if(wireframe_switch==1){
        vbo_mesh1.drawWireframe();
    }
    ofPopMatrix();
    
    /**unbind vertices**/
    if(inputswitch==0){
        //videoGrabber.getTextureReference().unbind();
    }
    if(inputswitch==1){
        if(hdmi_aspect_ratio_switch==0){
            cam1.getTexture().unbind();
        }
        if(hdmi_aspect_ratio_switch==1){
            aspect_fix_fbo.getTexture().unbind();
        }
    }
    
    shaderDisplace.end();    
    fbo0.end();
    
    /**draw to the screen**/
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2,ofGetHeight()/2,100);
    ofRotateXRad(rotate_x);
    ofRotateYRad(rotate_y);
    ofRotateZRad(rotate_capture_z);
    fbo0.draw(-ofGetWidth()/2+global_x_displace,-ofGetHeight()/2+global_y_displace,720,480);
    ofPopMatrix();
    
    //scale is the vertex resolution
    scale=d_scale;
    if(scale>=127){scale=127;}
    if(scale<=1){scale=1;}
    
    
    ofSetColor(255);
    //ofDrawBitmapString("fps =" + ofToString(ofGetFrameRate())+"fft low =" + ofToString(fft_low)+"fft low_smoothed =" + ofToString(low_value_smoothed)+"fft mid =" + ofToString(fft_low)+"fft mid_smoothed =" + ofToString(mid_value_smoothed), 10, ofGetHeight() - 5 );
    /*ofDrawBitmapString("fft high =" + ofToString(fft_high,2)+
     "fft high_smoothed =" + ofToString(high_value_smoothed,2)+
     "fft mid =" + ofToString(fft_mid,2)+
     "fft mid_smoothed =" + ofToString(mid_value_smoothed,2)+
     "fft low =" + ofToString(fft_low,2)+
     "fft low_smoothed =" + ofToString(low_value_smoothed,2)
     , 10, ofGetHeight() - 5 );
     */
    ofDrawBitmapString("plock0 = " + ofToString(p_lockSmoothed[0],2)+
                       ", midiActive0 = " +ofToString(midiActiveFloat[0],0)+
                       ", low0 = " + ofToString(fftLow[0],2)+
                       ", lowActive = " + ofToString(lowActiveFloat[0],0)+
                       //", mid0 = " + ofToString(fftMid[0],2)+
                       //", midActive = " + ofToString(midActiveFloat[0],0)+
                       ", high0 = " + ofToString(fftHigh[0],2)+
                       ", highActive = " + ofToString(highActiveFloat[0],0)
                       , 10, ofGetHeight() - 5 );
}

//+---------------------------------------------------------------

ofImage ofApp::perlin_noise(float theta, float resolution, ofImage noise_image){
    
    //float noiseScale = ofMap(mouseX, 0, ofGetWidth(), 0, 0.1);
    //float noiseVel = ofGetElapsedTimef();
    resolution=resolution*.05;
    theta=theta*.1;
    ofPixels & pixels = noise_image.getPixels();
    int w = noise_image.getWidth();
    int h = noise_image.getHeight();
    for(int y=0; y<h; y++) {
        for(int x=0; x<w; x++) {
            int i = y * w + x;
            float noiseVelue = ofNoise(x * resolution, y * resolution, theta);
            pixels[i] = 255 * noiseVelue;
        }
    }
    
    noise_image.update();
    return noise_image;
}
//--------------------------------------------------------------
void ofApp::exit() {
    // clean up
    midiIn.closePort();
    midiIn.removeListener(this);
}

//-----------------------------------------------------------
void ofApp::fftArrayClear(){
    for(int i=0;i<controlSize;i++){
        fftLow[i]=0;
        fftMid[i]=0;
        fftHigh[i]=0;
    }
    
}

//-------------------------------------------------------------
void ofApp::fftValuesUpdate(){
    //so we can use this to remap things regularly, or like just if you hit a switch to renormalize
    if(fft.getLowVal()>my_normalize){
        my_normalize=fft.getLowVal();
        //cout<<"highest possible"<<my_normalize<<endl;
    }
    
    fft_low=fft.getLowVal();
    fft_low=ofClamp(fft_low,1.0f,range);
    fft_low=fft_low/range;
    low_value_smoothed=fft_low*smoothing_rate+(1.0f-smoothing_rate)*low_value_smoothed;
    
    fft_mid=fft.getMidVal();
    fft_mid=ofClamp(fft_mid,1.0f,range);
    fft_mid=fft_mid/range;
    mid_value_smoothed=fft_mid*smoothing_rate+(1.0f-smoothing_rate)*mid_value_smoothed;
    
    fft_high=fft.getHighVal();
    fft_high=ofClamp(fft_high,1.0f,range);
    fft_high=fft_high/range;
    high_value_smoothed=fft_high*smoothing_rate+(1.0f-smoothing_rate)*high_value_smoothed;
    
}
//-------------------------------------------------------------
void ofApp::p_lockUpdate(){
    for(int i=0;i<p_lock_number;i++){
        p_lockSmoothed[i]=p_lock[i][p_lock_increment]*(1.0f-p_lock_smooth)+p_lockSmoothed[i]*p_lock_smooth;
        if(abs(p_lockSmoothed[i])<.01){p_lockSmoothed[i]=0;}
    }
    if(p_lock_record_switch==1){
        p_lock_increment++;
        p_lock_increment=p_lock_increment%p_lock_size;
    }
}

//------------------------------------------------------
void ofApp::p_lockClear(){
    for(int i=0;i<p_lock_number;i++){
        for(int j=0;j<p_lock_size;j++){
            p_lock[i][j]=0; 
        }//endplocksize
    }//endplocknumber
}

//--------------------------------------------------------------
void ofApp::newMidiMessage(ofxMidiMessage& msg) {
    
    // add the latest message to the message queue
    midiMessages.push_back(msg);
    
    // remove any old messages if we have too many
    while(midiMessages.size() > maxMessages) {
        midiMessages.erase(midiMessages.begin());
    }
}

//----------------------------------------------------------
void ofApp::midiSetup(){
    // print input ports to console
    midiIn.listInPorts();
    
    // open port by number (you may need to change this)
    midiIn.openPort(1);
    //midiIn.openPort("IAC Pure Data In");	// by name
    //midiIn.openVirtualPort("ofxMidiIn Input"); // open a virtual port
    
    // don't ignore sysex, timing, & active sense messages,
    // these are ignored by default
    midiIn.ignoreTypes(false, false, false);
    
    // add ofApp as a listener
    midiIn.addListener(this);
    
    // print received messages to the console
    midiIn.setVerbose(true);
}
//--------------------------------------------------------
void ofApp::clearMidiActiveFloat(){
    for(int i=0;i<17;i++){
        midiActiveFloat[i]=0;
    }
}
//--------------------------------------------------------
void ofApp::clearLowActiveFloat(){
    for(int i=0;i<17;i++){
        lowActiveFloat[i]=0;
    }
}
//--------------------------------------------------------
void ofApp::clearMidActiveFloat(){
    for(int i=0;i<17;i++){
        midActiveFloat[i]=0;
    }
}
//--------------------------------------------------------
void ofApp::clearHighActiveFloat(){
    for(int i=0;i<17;i++){
        highActiveFloat[i]=0;
    }
}	
//----------------------------------------------------------
void ofApp::midibiz(){
    for(unsigned int i = 0; i < midiMessages.size(); ++i) {
        
        ofxMidiMessage &message = midiMessages[i];
        
        if(message.status < MIDI_SYSEX) {
            //text << "chan: " << message.channel;
            if(message.status == MIDI_CONTROL_CHANGE) {
                
                //How to Midi Map
                //uncomment the line that says cout<<message control etc
                //run the code and look down in the console
                //when u move a knob on yr controller keep track of the number that shows up
                //that is the cc value of the knob
                //then go down to the part labled 'MIDIMAPZONE'
                //and change the numbers for each if message.control== statement to the values
                //on yr controller
                
                // cout << "message.control"<< message.control<< endl;
                // cout << "message.value"<< message.value<< endl;
                
                //MIDIMAPZONE
                //these are mostly all set to output bipolor controls at this moment (ranging from -1.0 to 1.0)
                //if u uncomment the second line on each of these if statements that will switch thems to unipolor
                //controls (ranging from 0.0to 1.0) if  you prefer
                
                
                //reset switch
                if(message.control==58){
                    if(message.value==127){
                        for(int i=0;i<p_lock_number;i++){
                            midiActiveFloat[i]=0;
                            lowActiveFloat[i]=0;
                            midActiveFloat[i]=0;
                            highActiveFloat[i]=0;
                            fftLow[i]=fftMid[i]=fftHigh[i]=0.0;
                            for(int j=0;j<p_lock_size;j++){   
                                p_lock[i][j]=0;
                            }  
                        }
                        global_y_displace=global_x_displace=rotate_y=rotate_x=0;   
                    }   
                }
                //low freq
                if(message.control==43){
                    if(message.value==127){
                        control_switch=1;
                        clearMidiActiveFloat();
                        clearMidActiveFloat();
                        clearHighActiveFloat();	
                    }
                    if(message.value==0){
                        control_switch=0; 
                        clearLowActiveFloat();
                    }
                }
                //mid
                if(message.control==44){
                    if(message.value==127){
                        control_switch=2;
                        clearMidiActiveFloat();
                        clearLowActiveFloat();
                        clearHighActiveFloat();	
                    }
                    if(message.value==0){
                        control_switch=0; 
                        clearMidActiveFloat();
                    }
                }
                if(message.control==42){
                    if(message.value==127){
                        control_switch=3;
                        clearMidiActiveFloat();
                        clearLowActiveFloat();
                        clearMidActiveFloat();
                    }
                    if(message.value==0){
                        control_switch=0; 
                        clearHighActiveFloat();
                    }
                }
                
                //mesh selection
                if(message.control==41){
                    if(message.value==127){
                        horizontal_linemesh(scale);
                    }
                    if(message.value==0){trianglemesh(scale);}   
                }
                if(message.control==45){
                    if(message.value==127){
                        vertical_linemesh(scale);
                    }
                    if(message.value==0){trianglemesh(scale);}
                }
                
                /**newMidiShits**/
                //lumakey
                if(message.control==16){
                    if(control_switch==0){	
                        if(p_lock_0_switch==1){
                            if(abs(message.value/127.0f-p_lock[0][p_lock_increment])<CONTROL_THRESHOLD){
                                midiActiveFloat[0]=1;
                            }
                            if(midiActiveFloat[0]==1){
                                p_lock[0][p_lock_increment]=message.value/127.0f;
                            }
                        }              
                    }
                    if(control_switch==1){
                        if(abs(message.value/127.0f-fftLow[0])<CONTROL_THRESHOLD){
                            lowActiveFloat[0]=1;
                        }
                        if(lowActiveFloat[0]==1){
                            fftLow[0]=message.value/127.0f;
                        }	
                    }
                    if(control_switch==2){
                        if(abs(message.value/127.0f-fftMid[0])<CONTROL_THRESHOLD){
                            midActiveFloat[0]=1;
                        }
                        if(midActiveFloat[0]==1){
                            fftMid[0]=message.value/127.0f;
                        }	
                    }
                    if(control_switch==3){
                        if(abs(message.value/127.0f-fftHigh[0])<CONTROL_THRESHOLD){
                            highActiveFloat[0]=1;
                        }
                        if(highActiveFloat[0]==1){
                            fftHigh[0]=message.value/127.0f;
                        }	
                    }
                }
                //xdisplace
                if(message.control==17){
                    if(control_switch==0){
                        if(p_lock_0_switch==1){
                            if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[1][p_lock_increment])<CONTROL_THRESHOLD){
                                midiActiveFloat[1]=1;
                            }
                            if(midiActiveFloat[1]==1){
                                p_lock[1][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                            }
                        }
                    }
                    if(control_switch==1){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftLow[1])<CONTROL_THRESHOLD){
                            lowActiveFloat[1]=1;
                        }
                        if(lowActiveFloat[1]==1){
                            fftLow[1]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }	
                    }
                    if(control_switch==2){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftMid[1])<CONTROL_THRESHOLD){
                            midActiveFloat[1]=1;
                        }
                        if(midActiveFloat[1]==1){
                            fftMid[1]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }	
                    if(control_switch==3){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftHigh[1])<CONTROL_THRESHOLD){
                            highActiveFloat[1]=1;
                        }
                        if(highActiveFloat[1]==1){
                            fftHigh[1]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }
                }
                //ydisplace
                if(message.control==18){
                    if(control_switch==0){
                        if(p_lock_0_switch==1){
                            if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[2][p_lock_increment])<CONTROL_THRESHOLD){
                                midiActiveFloat[2]=1;
                            }
                            if(midiActiveFloat[2]==1){
                                p_lock[2][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                            }
                        }
                    }
                    if(control_switch==1){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftLow[2])<CONTROL_THRESHOLD){
                            lowActiveFloat[2]=1;
                        }
                        if(lowActiveFloat[2]==1){
                            fftLow[2]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }	
                    }
                    if(control_switch==2){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftMid[2])<CONTROL_THRESHOLD){
                            midActiveFloat[2]=1;
                        }
                        if(midActiveFloat[2]==1){
                            fftMid[2]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }	
                    if(control_switch==3){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftHigh[2])<CONTROL_THRESHOLD){
                            highActiveFloat[2]=1;
                        }
                        if(highActiveFloat[2]==1){
                            fftHigh[2]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }
                }
                //zlfo frequency
                if(message.control==19){
                    if(control_switch==0){	
                        if(p_lock_0_switch==1){
                            if(abs(message.value/127.0f-p_lock[3][p_lock_increment])<CONTROL_THRESHOLD){
                                midiActiveFloat[3]=1;
                            }
                            if(midiActiveFloat[3]==1){
                                p_lock[3][p_lock_increment]=message.value/127.0f;
                            }
                        }              
                    }
                    if(control_switch==1){
                        if(abs(message.value/127.0f-fftLow[3])<CONTROL_THRESHOLD){
                            lowActiveFloat[3]=1;
                        }
                        if(lowActiveFloat[3]==1){
                            fftLow[3]=message.value/127.0f;
                        }	
                    }
                    if(control_switch==2){
                        if(abs(message.value/127.0f-fftMid[0])<CONTROL_THRESHOLD){
                            midActiveFloat[3]=1;
                        }
                        if(midActiveFloat[3]==1){
                            fftMid[3]=message.value/127.0f;
                        }	
                    }
                    if(control_switch==3){
                        if(abs(message.value/127.0f-fftHigh[0])<CONTROL_THRESHOLD){
                            highActiveFloat[3]=1;
                        }
                        if(highActiveFloat[3]==1){
                            fftHigh[3]=message.value/127.0f;
                        }	
                    }
                }
                //xlfo frequency
                if(message.control==20){
                    if(control_switch==0){
                        if(p_lock_0_switch==1){
                            if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[4][p_lock_increment])<CONTROL_THRESHOLD){
                                midiActiveFloat[4]=1;
                            }
                            if(midiActiveFloat[4]==1){
                                p_lock[4][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                            }
                        }
                    }
                    if(control_switch==1){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftLow[4])<CONTROL_THRESHOLD){
                            lowActiveFloat[4]=1;
                        }
                        if(lowActiveFloat[4]==1){
                            fftLow[4]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }	
                    }
                    if(control_switch==2){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftMid[4])<CONTROL_THRESHOLD){
                            midActiveFloat[4]=1;
                        }
                        if(midActiveFloat[4]==1){
                            fftMid[4]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }	
                    if(control_switch==3){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftHigh[4])<CONTROL_THRESHOLD){
                            highActiveFloat[4]=1;
                        }
                        if(highActiveFloat[4]==1){
                            fftHigh[4]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }
                }
                //yflo frequency
                if(message.control==21){
                    if(control_switch==0){
                        if(p_lock_0_switch==1){
                            if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[5][p_lock_increment])<CONTROL_THRESHOLD){
                                midiActiveFloat[5]=1;
                            }
                            if(midiActiveFloat[5]==1){
                                p_lock[5][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                            }
                        }
                    }
                    if(control_switch==1){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftLow[5])<CONTROL_THRESHOLD){
                            lowActiveFloat[5]=1;
                        }
                        if(lowActiveFloat[5]==1){
                            fftLow[5]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }	
                    }
                    if(control_switch==2){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftMid[5])<CONTROL_THRESHOLD){
                            midActiveFloat[5]=1;
                        }
                        if(midActiveFloat[5]==1){
                            fftMid[5]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }	
                    if(control_switch==3){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftHigh[5])<CONTROL_THRESHOLD){
                            highActiveFloat[5]=1;
                        }
                        if(highActiveFloat[5]==1){
                            fftHigh[5]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }
                }
                //zooming
                if(message.control==22){
                    if(control_switch==0){
                        if(p_lock_0_switch==1){
                            if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[6][p_lock_increment])<CONTROL_THRESHOLD){
                                midiActiveFloat[6]=1;
                            }
                            if(midiActiveFloat[6]==1){
                                p_lock[6][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                            }
                        }
                    }
                    if(control_switch==1){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftLow[6])<CONTROL_THRESHOLD){
                            lowActiveFloat[6]=1;
                        }
                        if(lowActiveFloat[6]==1){
                            fftLow[6]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }	
                    }
                    if(control_switch==2){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftMid[6])<CONTROL_THRESHOLD){
                            midActiveFloat[6]=1;
                        }
                        if(midActiveFloat[6]==1){
                            fftMid[6]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }	
                    if(control_switch==3){
                        if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-fftHigh[6])<CONTROL_THRESHOLD){
                            highActiveFloat[6]=1;
                        }
                        if(highActiveFloat[6]==1){
                            fftHigh[6]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                        }
                    }
                }
                //scaling/thickening
                if(message.control==23){
                    //actually midi latch and audio reactive doesn't even make sense for this control...
                    /*
                     if(abs(message.value/127.0f-p_lock[7][p_lock_increment])<CONTROL_THRESHOLD){
                     midiActiveFloat[7]=1;
                     }
                     if(midiActiveFloat[7]==1){*/
                    p_lock[7][p_lock_increment]=message.value/127.0f;
                    /*
                     }
                     if(stroke_weight_switch==1){
                     stroke_weight=5*(message.value)/127.0f;
                     }*/
                }
                
                /*
                 //continuous controls
                 if(message.control==16){
                 if(abs(message.value/127.0f-p_lock[0][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[0]=1;
                 }
                 if(midiActiveFloat[0]==1){
                 p_lock[0][p_lock_increment]=message.value/127.0f;
                 }
                 }
                 if(message.control==17){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[1][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[1]=1;
                 }
                 if(midiActiveFloat[1]==1){
                 p_lock[1][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }	
                 }
                 if(message.control==18){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[2][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[2]=1;
                 }
                 if(midiActiveFloat[2]==1){
                 p_lock[2][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }	
                 } 
                 */
                
                
                /*
                 if(message.control==60){
                 if(message.value==127){
                 p_lock_record_switch=1;
                 for(int i=0;i<p_lock_number;i++){
                 p_lockSmoothed[i]=0;
                 for(int j=0;j<p_lock_size;j++){
                 p_lock[i][j]=p_lock[i][p_lock_increment];    
                 }    
                 }   
                 }
                 if(message.value==0){
                 p_lock_record_switch=0;
                 }
                 }
                 //reset switch
                 if(message.control==58){
                 if(message.value==127){
                 for(int i=0;i<p_lock_number;i++){
                 for(int j=0;j<p_lock_size;j++){   
                 p_lock[i][j]=0;
                 midiActiveFloat[i]=0;
                 }  
                 }
                 global_y_displace=global_x_displace=rotate_y=rotate_x=0;   
                 }   
                 }
                 //continuous controls
                 if(message.control==16){
                 if(abs(message.value/127.0f-p_lock[0][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[0]=1;
                 }
                 if(midiActiveFloat[0]==1){
                 p_lock[0][p_lock_increment]=message.value/127.0f;
                 }
                 }
                 if(message.control==17){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[1][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[1]=1;
                 }
                 if(midiActiveFloat[1]==1){
                 p_lock[1][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }	
                 }
                 if(message.control==18){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[2][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[2]=1;
                 }
                 if(midiActiveFloat[2]==1){
                 p_lock[2][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }	
                 }
                 if(message.control==19){
                 if(abs(message.value/127.0f-p_lock[3][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[3]=1;
                 }
                 if(midiActiveFloat[3]==1){
                 p_lock[3][p_lock_increment]=message.value/127.0f;
                 }
                 }
                 if(message.control==20){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[4][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[4]=1;
                 }
                 if(midiActiveFloat[4]==1){
                 p_lock[4][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }	
                 }
                 if(message.control==21){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[5][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[5]=1;
                 }
                 if(midiActiveFloat[5]==1){
                 p_lock[5][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }	
                 }
                 if(message.control==22){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[6][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[6]=1;
                 }
                 if(midiActiveFloat[6]==1){
                 p_lock[6][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }	
                 }
                 if(message.control==23){
                 if(abs(message.value/127.0f-p_lock[7][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[7]=1;
                 }
                 if(midiActiveFloat[7]==1){
                 p_lock[7][p_lock_increment]=message.value/127.0f;
                 }
                 if(stroke_weight_switch==1){
                 stroke_weight=5*(message.value)/127.0f;
                 }
                 }
                 //multi_x_stuffs
                 if(message.control==32){
                 if(message.value==127){
                 global_x_displace_switch=0;
                 center_x_displace_switch=1;
                 midiActiveFloat[8]=0;
                 }
                 if(message.value==0){
                 global_x_displace_switch=1;
                 center_x_displace_switch=0;
                 }
                 }
                 if(message.control==48){
                 if(message.value==127){
                 center_x_displace_switch=1;
                 rotate_x_switch=0;
                 global_x_displace_switch=1;
                 midiActiveFloat[8]=0;
                 }
                 if(message.value==0){
                 rotate_x_switch=1;
                 center_x_displace_switch=0;
                 }
                 }
                 if(message.control==64){
                 if(message.value==127){
                 rotate_capture_z_switch=0;
                 center_x_displace_switch=1;
                 rotate_x_switch=1;
                 global_x_displace_switch=1;
                 midiActiveFloat[8]=0;
                 }
                 if(message.value==0){
                 rotate_capture_z_switch=1;
                 center_x_displace_switch=0;
                 }
                 }
                 //so c9 and c10 will be multipurpose switches
                 if(message.control==120){
                 if(center_x_displace_switch==0){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[8][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[8]=1;
                 }
                 if(midiActiveFloat[8]==1){
                 p_lock[8][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }	
                 }
                 if(global_x_displace_switch==0){
                 global_x_displace=ofGetWidth()/4*(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 if(rotate_x_switch==0){
                 rotate_x=3.14*(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 if(rotate_capture_z_switch==0){
                 rotate_capture_z=3.14*(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }   
                 }
                 //multi_y_stuffs
                 if(message.control==33){
                 if(message.value==127){
                 global_y_displace_switch=0;
                 center_y_displace_switch=1;
                 midiActiveFloat[9]=0;
                 }	
                 if(message.value==0){
                 global_y_displace_switch=1;
                 center_y_displace_switch=0;
                 }
                 }
                 if(message.control==49){
                 if(message.value==127){
                 center_y_displace_switch=1;
                 rotate_y_switch=0;
                 global_y_displace_switch=1;
                 midiActiveFloat[9]=0;
                 }
                 if(message.value==0){
                 rotate_y_switch=1;
                 center_y_displace_switch=0;
                 }
                 }
                 if(message.control==121){
                 
                 if(center_y_displace_switch==0){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[9][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[9]=1;
                 }
                 if(midiActiveFloat[9]==1){
                 p_lock[9][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 }
                 if(global_y_displace_switch==0){
                 global_y_displace=ofGetHeight()/4*(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 if(rotate_y_switch==0){
                 rotate_y=3.14*(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 
                 }
                 if(message.control==122){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[10][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[10]=1;
                 }
                 if(midiActiveFloat[10]==1){
                 p_lock[10][p_lock_increment]=.1*(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 if(z_freq0==TRUE){
                 p_lock[10][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 }
                 }
                 if(message.control==123){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[11][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[11]=1;
                 }
                 if(midiActiveFloat[11]==1){
                 p_lock[11][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 }
                 if(message.control==124){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[12][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[12]=1;
                 }
                 if(midiActiveFloat[12]==1){
                 p_lock[12][p_lock_increment]=.1*(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 if(x_freq0==TRUE){
                 p_lock[12][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 }     
                 }
                 if(message.control==125){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[13][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[13]=1;
                 }
                 if(midiActiveFloat[13]==1){
                 p_lock[13][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 }
                 if(message.control==126){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[14][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[14]=1;
                 }
                 if(midiActiveFloat[14]==1){
                 p_lock[14][p_lock_increment]=.1*(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 if(y_freq0==TRUE){
                 p_lock[14][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 }          
                 }
                 if(message.control==127){
                 if(abs((message.value-MIDI_MAGIC)/MIDI_MAGIC-p_lock[15][p_lock_increment])<CONTROL_THRESHOLD){
                 midiActiveFloat[15]=1;
                 }
                 if(midiActiveFloat[15]==1){
                 p_lock[15][p_lock_increment]=(message.value-MIDI_MAGIC)/MIDI_MAGIC;
                 }
                 }
                 //frequency switches
                 //y
                 if(message.control==38){
                 if(message.value==127){
                 y_freq0=TRUE;
                 }
                 if(message.value==0){
                 y_freq0=FALSE;
                 }
                 }
                 if(message.control==54){
                 if(message.value==127){
                 y_ringmod_switch=TRUE;
                 }   
                 if(message.value==0){
                 y_ringmod_switch=FALSE;
                 }
                 }
                 if(message.control==70){
                 if(message.value==127){
                 y_phasemod_switch=TRUE;
                 }   
                 if(message.value==0){
                 y_phasemod_switch=FALSE;
                 }
                 }
                 //x
                 if(message.control==36){
                 if(message.value==127){
                 x_freq0=TRUE;
                 }
                 if(message.value==0){
                 x_freq0=FALSE;
                 }   
                 }
                 if(message.control==52){
                 if(message.value==127){
                 x_ringmod_switch=TRUE;
                 }   
                 if(message.value==0){
                 x_ringmod_switch=FALSE;
                 }
                 }
                 if(message.control==68){
                 if(message.value==127){
                 x_phasemod_switch=TRUE;
                 }
                 
                 if(message.value==0){
                 x_phasemod_switch=FALSE;
                 }
                 }
                 //z
                 if(message.control==34){
                 if(message.value==127){
                 z_freq0=TRUE;
                 }
                 
                 if(message.value==0){
                 z_freq0=FALSE;
                 }
                 }
                 if(message.control==50){
                 if(message.value==127){
                 z_ringmod_switch=TRUE;
                 }   
                 if(message.value==0){
                 z_ringmod_switch=FALSE;
                 }
                 }
                 if(message.control==66){
                 if(message.value==127){
                 z_phasemod_switch=TRUE;
                 }   
                 if(message.value==0){
                 z_phasemod_switch=FALSE;
                 }
                 }
                 //yLfoShapees
                 //yLfoShape=0;
                 if(message.control==39){
                 if(message.value==127){
                 yLfoShape=1;
                 
                 }
                 if(message.value==0){
                 yLfoShape=0;
                 
                 }
                 }
                 if(message.control==55){
                 if(message.value==127){
                 yLfoShape=2;
                 
                 }
                 if(message.value==0){
                 yLfoShape=0; 
                 }   
                 }
                 if(message.control==71){
                 if(message.value==127){
                 yLfoShape=3;
                 }
                 if(message.value==0){
                 yLfoShape=0;  
                 }   
                 }
                 if(message.control==37){
                 if(message.value==127){
                 xLfoShape=1;
                 }
                 if(message.value==0){
                 xLfoShape=0;   
                 }   
                 }
                 if(message.control==53){
                 if(message.value==127){
                 xLfoShape=2;
                 }
                 if(message.value==0){
                 xLfoShape=0;   
                 }   
                 }
                 if(message.control==69){
                 if(message.value==127){
                 xLfoShape=3;
                 
                 }
                 if(message.value==0){
                 xLfoShape=0;
                 
                 }   
                 }
                 if(message.control==35){
                 if(message.value==127){
                 zLfoShape=1;
                 
                 }
                 if(message.value==0){
                 zLfoShape=0;
                 
                 }   
                 }               
                 if(message.control==51){
                 if(message.value==127){
                 zLfoShape=2;
                 
                 }
                 if(message.value==0){
                 zLfoShape=0;
                 
                 }
                 
                 }
                 if(message.control==67){
                 if(message.value==127){
                 zLfoShape=3;
                 }
                 if(message.value==0){
                 zLfoShape=0; 
                 }
                 }
                 
                 //greyscale
                 if(message.control==46){
                 if(message.value==127){
                 b_w_switch=1;     
                 }
                 if(message.value==0){
                 b_w_switch=0;    
                 }
                 }
                 
                 if(message.control==43){
                 if(message.value==127){
                 trianglemesh(scale);
                 wireframe_switch=FALSE;
                 }
                 }
                 if(message.control==44){
                 if(message.value==127){
                 horizontal_linemesh(scale);
                 }   
                 }
                 if(message.control==42){
                 if(message.value==127){
                 vertical_linemesh(scale);
                 }
                 }
                 if(message.control==41){
                 if(message.value==127){
                 trianglemesh(scale);
                 wireframe_switch=TRUE;
                 }
                 }
                 if(message.control==45){
                 if(message.value==127){
                 stroke_weight_switch=1;
                 midiActiveFloat[7]=0;
                 }
                 if(message.value==0){   
                 stroke_weight_switch=0;
                 }
                 }
                 if(message.control==61){
                 if(message.value==127){   
                 bright_switch=1;
                 }
                 if(message.value==0){
                 bright_switch=0;
                 }  
                 }
                 if(message.control==59){
                 if(message.value==127){
                 
                 invert_switch=1;
                 }
                 if(message.value==0){
                 
                 invert_switch=0;
                 } 
                 }
                 if(message.control==62){
                 if(message.value==127){
                 hdmi_aspect_ratio_switch=1;
                 //luma_switch=1;
                 }
                 if(message.value==0){
                 hdmi_aspect_ratio_switch=0;
                 //luma_switch=0;
                 }  
                 */
                
            }
        }
    }
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == ')') {fft.setNormalize(true);}
    
}
//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
}

//--------------------------------------------------------------
void ofApp::cameraSetup(int w, int h){
    /*omx_settings();	
     if(inputswitch==0){
     videoGrabber.setup(settings);
     }
     */
    if(inputswitch==1){
        cam1.setDesiredFrameRate(30);
        cam1.initGrabber(w,h);
    }
}

//-------------------------------------------------------------
void ofApp::cameraUpdate(){
    if(inputswitch==1){
        cam1.update();
        //corner crop and stretch to preserve hd aspect ratio
        if(hdmi_aspect_ratio_switch==1){
            aspect_fix_fbo.begin();
            cam1.draw(0,0,853,480);
            aspect_fix_fbo.end();	
        }
    }
    //if(inputswitch==0){omx_updates();}
}


//this makes a mesh of a grid
//but not quite
void ofApp:: gridmesh(int gridsize){
    mesh1.clearVertices();
    mesh1.clearTexCoords();
    ofVec3f Q_vertex1;
    ofVec3f Q_vertex2;
    ofVec3f Q_vertex3;
    ofVec3f Q_vertex4;
    
    ofVec2f Q_tex1;
    ofVec2f Q_tex2;
    ofVec2f Q_tex3;
    ofVec2f Q_tex4;
    
    for(int i=0;i<gridsize;i++){
        for(int j=0;j<gridsize;j++){
            int x0=j*width/gridsize;
            int x1=(j+1)*width/gridsize;
            int y0=i*height/gridsize;
            int y1=(i+1)*height/gridsize;
            
            Q_vertex1.set(x0,y0,0);
            Q_vertex2.set(x1,y0,0);
            Q_vertex3.set(x1,y1,0);
            Q_vertex4.set(x0,y1,0);
            
            Q_tex1.set(x0,y0);
            Q_tex2.set(x1,y0);
            Q_tex3.set(x1,y1);
            Q_tex4.set(x0,y1);
            
            
            
            mesh1.addVertex(Q_vertex1);
            mesh1.addVertex(Q_vertex2);
            mesh1.addVertex(Q_vertex4);
            mesh1.addVertex(Q_vertex3);
            
            //then corresponding texture coordinates one for each vertext
            mesh1.addTexCoord(Q_tex1);
            mesh1.addTexCoord(Q_tex2);
            mesh1.addTexCoord(Q_tex4);
            mesh1.addTexCoord(Q_tex3);
            
        }//endxfor
    }//endyfor
    mesh1.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    vbo_mesh1= mesh1;
    
    
}
//this makes a mesh of some loose triangles
void ofApp:: trianglemesh(int gridsize){
    mesh1.clearVertices();
    mesh1.clearTexCoords();
    ofVec3f Q_vertex1;
    ofVec3f Q_vertex2;
    ofVec3f Q_vertex3;
    ofVec3f Q_vertex4;
    
    ofVec2f Q_tex1;
    ofVec2f Q_tex2;
    ofVec2f Q_tex3;
    ofVec2f Q_tex4;
    
    for(int i=0;i<gridsize;i++){
        for(int j=0;j<gridsize;j++){
            int x0=j*width/gridsize;
            int x1=(j+1)*width/gridsize;
            int y0=i*height/gridsize;
            int y1=(i+1)*height/gridsize;
            
            Q_vertex1.set(x0,y0,0);
            Q_vertex2.set(x1,y0,0);
            Q_vertex3.set(x1,y1,0);
            Q_vertex4.set(x0,y1,0);
            
            
            //so if i go from 0-1 then 
            //the tex coords are still only in the 
            //bottom left hand corner
            //only integer values make adifference 
            //for rescaling
            /*
             float tex_x0=j/gridsize*rescale;
             float tex_x1=(j+1)/gridsize*rescale;
             float tex_y0=i/gridsize*rescale;
             float tex_y1=(i+1)/gridsize*rescale;\
             */
            
            //if i do this then it moves to the 
            //upper right hand corner
            // then if rescale=1/gridsize it fills
            rescale=1.0/gridsize;
            float tex_x0=j*rescale;
            float tex_x1=(j+1)*rescale;
            float tex_y0=i*rescale;
            float tex_y1=(i+1)*rescale;
            
            
            
            Q_tex1.set(tex_x0,tex_y0);
            Q_tex2.set(tex_x1,tex_y0);
            Q_tex3.set(tex_x1,tex_y1);
            Q_tex4.set(tex_x0,tex_y1);
            
            
            
            mesh1.addVertex(Q_vertex1);
            mesh1.addVertex(Q_vertex2);
            mesh1.addVertex(Q_vertex3);
            
            
            
            mesh1.addVertex(Q_vertex3);
            mesh1.addVertex(Q_vertex4);
            mesh1.addVertex(Q_vertex1);
            
            //then corresponding texture coordinates one for each vertext
            mesh1.addTexCoord(Q_tex1);
            mesh1.addTexCoord(Q_tex2);
            mesh1.addTexCoord(Q_tex3);
            
            mesh1.addTexCoord(Q_tex3);
            mesh1.addTexCoord(Q_tex4);
            mesh1.addTexCoord(Q_tex1);
        }//endxfor
    }//endyfor
    mesh1.setMode(OF_PRIMITIVE_TRIANGLES);
    vbo_mesh1= mesh1;
    
    
}

//this makes a mesh of a line horizontal
void ofApp:: horizontal_linemesh(int gridsize){
    int new_gridsize=gridsize*2;
    mesh1.clearVertices();
    mesh1.clearTexCoords();
    ofVec3f Q_vertex1;
    ofVec3f Q_vertex2;
    
    
    ofVec2f Q_tex1;
    ofVec2f Q_tex2;
    
    for(int i=0;i<new_gridsize;i++){
        for(int j=0;j<new_gridsize;j++){
            int x0=j*width/new_gridsize;
            int x1=(j+1)*width/new_gridsize;
            int y0=i*height/new_gridsize;
            // int y1=(i+1)*height/gridsize;
            
            Q_vertex1.set(x0,y0,0);
            Q_vertex2.set(x1,y0,0);
            
            
            rescale=1.0/new_gridsize;
            float tex_x0=j*rescale;
            float tex_x1=(j+1)*rescale;
            float tex_y0=i*rescale;
            Q_tex1.set(tex_x0,tex_y0);
            Q_tex2.set(tex_x1,tex_y0);
            
            
            
            mesh1.addVertex(Q_vertex1);
            mesh1.addVertex(Q_vertex2);
            
            
            //then corresponding texture coordinates one for each vertext
            mesh1.addTexCoord(Q_tex1);
            mesh1.addTexCoord(Q_tex2);
            
        }//endxfor
    }//endyfor
    mesh1.setMode(OF_PRIMITIVE_LINES);
    
    vbo_mesh1= mesh1;
    
    
}

//this makes a mesh of a line vertical
void ofApp:: vertical_linemesh(int gridsize){
    int new_gridsize=gridsize*2;
    mesh1.clearVertices();
    mesh1.clearTexCoords();
    ofVec3f Q_vertex1;
    ofVec3f Q_vertex2;
    
    
    ofVec2f Q_tex1;
    ofVec2f Q_tex2;
    
    for(int i=0;i<new_gridsize;i++){
        for(int j=0;j<new_gridsize;j++){
            int x0=i*width/new_gridsize;
            // int x1=(i+1)*width/gridsize;
            int y0=j*height/new_gridsize;
            int y1=(j+1)*height/new_gridsize;
            
            Q_vertex1.set(x0,y0,0);
            Q_vertex2.set(x0,y1,0);
            
            rescale=1.0/new_gridsize;
            float tex_x0=i*rescale;
            float tex_y0=j*rescale;
            float tex_y1=(j+1)*rescale;
            
            Q_tex1.set(tex_x0,tex_y0);
            Q_tex2.set(tex_x0,tex_y1);
            
            
            
            mesh1.addVertex(Q_vertex1);
            mesh1.addVertex(Q_vertex2);
            
            
            //then corresponding texture coordinates one for each vertext
            mesh1.addTexCoord(Q_tex1);
            mesh1.addTexCoord(Q_tex2);
            
        }//endxfor
    }//endyfor
    mesh1.setMode(OF_PRIMITIVE_LINES);
    
    vbo_mesh1= mesh1;
    
    
}
//------------------------------------------------------

void ofApp::allocateFbo(){
    aspect_fix_fbo.allocate(width,height);
    aspect_fix_fbo.begin();
    ofClear(0,0,0,255);
    aspect_fix_fbo.end();
    
    //framebufferbiz
    fbo0.allocate(width,height);
    fbo0.begin();
    ofClear(0,0,0,255);
    fbo0.end();
}
