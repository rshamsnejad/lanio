#gst-launch-1.0 sdpsrc location="sdp://$1" ! decodebin ! autoaudiosink

gst-launch-1.0 sdpsrc location="sdp://$1" ! decodebin ! audioconvert ! audioresample ! alsasink device=hw:CARD=I82801AAICH,DEV=0
