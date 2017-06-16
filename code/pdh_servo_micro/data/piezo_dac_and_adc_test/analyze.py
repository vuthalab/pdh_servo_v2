import numpy as np
import matplotlib.pyplot as plt

dac_word, adc_word = np.loadtxt('piezo_read.txt').T

# with open('piezo_read.txt', 'r') as fp:
#     dat = fp.readlines()

dac_useful_low = 2500
dac_useful_high = 56000

use_data = (dac_word > dac_useful_low) * (dac_word < dac_useful_high)

dac_word_use = dac_word[use_data]
adc_word_use = adc_word[use_data]
p_fit = np.polyfit(dac_word_use, adc_word_use, 1)

plt.figure(figsize=(8, 6))
plt.plot(dac_word, adc_word)
plt.plot(dac_word, np.poly1d(p_fit)(dac_word))
plt.xlabel('DAC word')
plt.ylabel('ADC word')
plt.title(str(p_fit))
plt.xticks(rotation=45)
plt.tight_layout()
plt.savefig('adc_word_vs_dacword.png')
