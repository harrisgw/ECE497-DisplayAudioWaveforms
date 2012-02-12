#!/bin/sh
port=1234
echo "How many streams (enter:1 or 2)?"
read stream
if [ "$stream" -eq "2" ];then
	echo "Would you like those streams in 1 window or 2 windows? (enter:1 or 2)"
	read win
	if [ "$win" -eq "2" ];then
		window=2
	else
		window=1
	fi
else
	window=1
fi
echo "How many samples per window?"
read samples
echo "Enter Minimum Y axis:"
read miny
echo "Enter Maximum Y axis:"
read maxy
echo "Enter window width:"
read width
echo "Enter window height:"
read height
echo "Enter IP address of Beagle:"
read beagle

echo "window=$window, stream=$stream, samples=$samples ymin=$miny ymax=$maxy width=$width height=$height beagleIP=$beagle"

if [ "$stream" -eq "2" ];then
	if [ "$window" -eq "2" ];then
		nc $beagle $port | perl ./driveGnuPlotStreams.pl $stream $window $samples $samples $miny $maxy $miny $maxy $width"x"$height"+0+0" $width"x"$height"+$width+0" 'channel1' 'channel2' 0 1
	else
		nc $beagle $port | perl ./driveGnuPlotStreams.pl $stream $window $samples $miny $maxy $width"x"$height"+0+0" 'channel1' 'channel2' 0 0
	fi
else
	nc $beagle $port | perl ./driveGnuPlotStreams.pl $stream $window $samples $miny $maxy $width"x"$height"+0+0" 'channel1' 0 
fi
