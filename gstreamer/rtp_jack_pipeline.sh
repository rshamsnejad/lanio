gst-launch-1.0 udpsrc port=5004 address=239.69.178.105 multicast-iface=enp0s8 caps="application/x-rtp, media=(string)audio, clock-rate=(int)48000, encoding-name=(string)L24, channels=(int)2, payload=(int)97" ! rtpbin latency=0 ! rtpL24depay ! audioconvert ! jackaudiosink name="Dante Stream" client-name="Linux AES67" connect=0
