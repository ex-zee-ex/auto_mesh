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
#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
//#include "ofxOMXVideoGrabber.h"
#include "ofxProcessFFT.h"

class ofApp : public ofBaseApp, public ofxMidiListener {
	
public:
	
	void setup();
	void update();
	void draw();
	void exit();
	
	void keyPressed(int key);
	void keyReleased(int key);
	
	/*
	void omx_settings();
	void omx_updates();
	ofxOMXCameraSettings settings;
    ofxOMXVideoGrabber videoGrabber;
	*/
	
	void midibiz();
	void midiSetup();
	void newMidiMessage(ofxMidiMessage& eventArgs);
	ofxMidiIn midiIn;
	std::vector<ofxMidiMessage> midiMessages;
	std::size_t maxMessages = 10; //< max number of messages to keep track of

	//p_lock shit
	void p_lockClear();
	void p_lockUpdate();
	
	//fft shit
	void fftArrayClear();
	void fftValuesUpdate();
    ProcessFFT fft;
    
    //shader
    ofShader shaderDisplace;
    
    //spectral mesh stuff
    void gridmesh(int gridsize);
    void trianglemesh(int gridsize);
    void horizontal_linemesh(int gridsize);
    void vertical_linemesh(int gridsize);
    
    //double check that both of these are necessary
    ofMesh mesh1;
    ofVboMesh vbo_mesh1;
    
    //video inputs
    ofVideoGrabber cam1;
    void cameraSetup(int w, int h);
	void cameraUpdate();
	
	//fbo
	ofFbo fbo0;
	ofFbo aspect_fix_fbo;
    void allocateFbo();
    
    ofImage perlin_noise(float theta, float resolution, ofImage noise_image);  
    ofImage x_noise_image;
    ofImage y_noise_image;
    ofImage z_noise_image;
    
    //midi latching
    void clearMidiActiveFloat();
    void clearLowActiveFloat();
    void clearMidActiveFloat();
    void clearHighActiveFloat();
    
    //keyboard controls
	float ss=1;
	float dd=1;
	float gg=1;
	float hh=0;
	float ee=1;
	float pp=1;
	float ii=.01;
	float az=0.0;
	float sx=0.0;
	float dc=0.0;
	float fv=.0;
	float gb=.0;
	float hn=.0;
	float jm=.0;
	float kk=.0;
	float ll=.0;
	float ylfo_amp=0.0;
	float ty=.0;
	float qw=0;
	float er=0;
	float ui=0;
	float op=0;
	
};
