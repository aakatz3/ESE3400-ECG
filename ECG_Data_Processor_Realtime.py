# ESE340 Heartpy Continuous Data Processor
import traceback
import heartpy as hp
import numpy as np
from scipy.signal import resample
import matplotlib.pyplot as plt
import serial

# Parameters - 5 Second sample time per shot, sample delay on nRF52 0.01s
sample_time = 5
sample_delay = 0.0107 # Tweaking this a little to account for inaccurate timer helps
sample_rate = 1/sample_delay
# Print statistics
print(f'Sample rate: {sample_rate}, Sample time: {sample_time}')
# Initialize data buffer
data = np.zeros(int(np.round(sample_time/sample_delay)))
# Open Serial Port ot reciever nRF52
ser = serial.Serial('COM6', 115200, timeout=1)

try:
    # Repeat analysis forever
    while True:
        # Read in all data, split off at the : and save the second part
        for i in np.arange(data.size):
            data[i] = ser.readline().decode("utf-8").split(':')[1]
            #print(data[i])
            #print(i)

        # Setup a plot for raw data and plot it
        f = plt.figure(figsize=(12, 4))
        plt.plot(np.arange(len(data)),data)
        plt.xlabel('Sample #')
        plt.ylabel('ADC Counts')
        plt.title(f'Raw Data')
        f.show()

        # Resample: upsample by 2
        multiplier = 2

        # Filter out baseline (or bypass baseline filter)
        filtered_baseline = hp.remove_baseline_wander(data, sample_rate, cutoff=0.05)
        # filtered_baseline = data

        # Filter out T wave for better R-R detection (or bypass)
        filtered = hp.filter_signal(filtered_baseline, cutoff=.05, sample_rate=sample_rate, filtertype='notch')
        # filtered = filtered_baseline

        # Upsample data
        resampled_data = resample(filtered, len(filtered) * multiplier)

        # And run the analysis again. Don't forget to up the sample rate as well!
        wd, m = hp.process(hp.scale_data(resampled_data), sample_rate * multiplier, high_precision=False, high_precision_fs=100)
        # wd, m = hp.process(hp.scale_data(filtered), sample_rate, high_precision=False, high_precision_fs=1000)

        # visualise in plot of custom size
        plt.figure()
        hp.plotter(wd, m, figsize=(12, 4))

        # display computed measures
        for measure in m.keys():
            print('%s: %f' % (measure, m[measure]))
except KeyboardInterrupt:
    print('Aborting analysis')
except Exception as err:
    # Print errors if we crash
    print(traceback.format_exception(err))
finally:
    # Close serial port and exit
    ser.close()
    from sys import exit
    exit()
