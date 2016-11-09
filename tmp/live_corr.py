#!/usr/bin/python

import numpy as np
import matplotlib.pyplot as plt
import threading
import sys

nsamples = 16384

# ugliness ahead
def run():
	plt.ion()
	plt.figure(figsize=(20, 20))

	k = [np.swapaxes(np.fromfile(sys.stdin, dtype=(np.complex64, 4), count=nsamples), 0, 1)]

	i = np.fft.fft(k[0] * np.hamming(nsamples))

	subplots = []
	plots = []

	for y in xrange(4):
		for x in xrange(4):
			subplots.append(plt.subplot(4, 4, 1 + y * 4 + x))
			cor = np.abs(np.fft.ifft(np.concatenate((i[y] * np.conjugate(i[x]), np.zeros(len(i[0])*15)))))
			cor = np.concatenate((cor[len(cor)/2:],cor[:len(cor)/2]))
			plt.title("%f" % (float(np.argmax(cor[len(cor)/2-80:len(cor)/2+80])-80)/16))
			plots.append(plt.plot(cor[len(cor)/2-80:len(cor)/2+80])[0])

	def read_thread():
		while True:
			k[0] = np.swapaxes(np.fromfile(sys.stdin, dtype=(np.complex64, 4), count=nsamples), 0, 1)

	read_thread = threading.Thread(target=read_thread)
	read_thread.daemon = True
	read_thread.start()

	while True:
		print "ah"
		i = np.fft.fft(k[0] * np.hamming(nsamples))

		print "eh"
		for y in xrange(4):
			for x in xrange(4):
				cor = np.abs(np.fft.ifft(np.concatenate((i[y] * np.conjugate(i[x]), np.zeros(len(i[0])*15)))))
				cor = np.concatenate((cor[len(cor)/2:],cor[:len(cor)/2]))
				subplots[y * 4 + x].set_title("upd %f" % (float(np.argmax(cor[len(cor)/2-80:len(cor)/2+80])-80)/16))
				plots[y * 4 + x].set_ydata(cor[len(cor)/2-80:len(cor)/2+80])

		print "ih"

		plt.pause(0.1)

if __name__ == "__main__":
	run()
