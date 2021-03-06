//OpenLRSng adaptive channel picker

// #define DEBUG_PICKER

inline void swap(uint8_t *a, uint8_t i, uint8_t j)
{
  uint8_t c = a[i];
  a[i] = a[j];
  a[j] = c;
}

inline void isort(uint8_t *a, uint8_t n)
{
  for (uint8_t i = 1; i < n; i++) {
    for (uint8_t j = i; j > 0 && a[j] < a[j-1]; j--) {
      swap(a, j, j - 1);
    }
  }
}

uint8_t chooseChannelsPerRSSI()
{
  static uint8_t chRSSImax[255];
  uint8_t picked[20];
  uint8_t n;

  for (n = 0; (n < MAXHOPS) && (bind_data.hopchannel[n] != 0); n++);

  printStr("Entering adaptive channel selection, picking: ");
  printULLn(n);
  init_rfm(0);
  rx_reset();
  for (uint8_t ch=1; ch<255; ch++) {
    printC('(');
    printUL(ch);
    printStr("/255)\r");

    uint32_t start = millis();
#ifdef DEBUG_PICKER
    printULLn(bind_data.rf_frequency + (uint32_t)ch * (uint32_t)bind_data.rf_channel_spacing * 10000UL);
#endif
    if ((bind_data.rf_frequency + (uint32_t)ch * (uint32_t)bind_data.rf_channel_spacing * 10000UL) > bind_data.maxFrequency) {
      chRSSImax[ch] = 255;
      continue; // do not break so we set all maxes to 255 to block them out
    }
    rfmSetChannel(ch);
    delay(1);
    chRSSImax[ch] = 0;
    while ((millis() - start) < 500) {
      uint8_t rssi = rfmGetRSSI();
      if (rssi > chRSSImax[ch]) {
        chRSSImax[ch] = rssi;
      }
    }
    if (ch & 1) {
      Green_LED_OFF
      Red_LED_ON
    } else {
      Green_LED_ON
      Red_LED_OFF
    }
  }

#ifdef DEBUG_PICKER
  for (uint8_t ch=1; ch<255; ch++) {
    printUL(ch);
    printC(',');
    printULLn(chRSSImax[ch]);
  }
#endif

  for (uint8_t i = 0; i < n; i++) {
    uint8_t lowest = 1, lowestRSSI = 255;
    for (uint8_t ch = 1; ch < 255; ch++) {
      if (chRSSImax[ch] < lowestRSSI) {
        lowestRSSI = chRSSImax[ch];
        lowest = ch;
      }
    }
    picked[i] = lowest;
    chRSSImax[lowest] = 255;
    if (lowest > 1) {
      chRSSImax[lowest - 1]=255;
    }
    if (lowest > 2) {
      chRSSImax[lowest - 2]=200;
    }
    if (lowest < 254) {
      chRSSImax[lowest + 1]=255;
    }
    if (lowest < 253) {
      chRSSImax[lowest + 2]=200;
    }
  }

#ifdef DEBUG_PICKER
  for (uint8_t i=0; i<n; i++) {
    printUL(picked[i]);
    printC(',');
  }
  printLf();
#endif

  isort(picked, n);

#ifdef DEBUG_PICKER
  for (uint8_t i=0; i<n; i++) {
    printUL(picked[i]);
    printC(',');
  }
  printLf();
#endif

  // this is empirically a decent way to shuffle changes to give decent hops
  for (uint8_t i = 0; i < (n / 2); i += 2) {
    swap(picked, i, i + n / 2);
  }

#ifdef DEBUG_PICKER
  for (uint8_t i=0; i<n; i++) {
    printUL(picked[i]);
    printC(',');
  }
  printLf();
#endif

  for (uint8_t i = 0; i < n; i++) {
    bind_data.hopchannel[i] = picked[i];
  }
  return 1;
}
