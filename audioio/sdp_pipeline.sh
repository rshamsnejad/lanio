gst-launch-1.0 sdpsrc location="sdp://$1" ! decodebin ! autoaudiosink
