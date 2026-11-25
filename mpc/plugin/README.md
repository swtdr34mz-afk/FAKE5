# FAKE5 for the MPC

The FAKE5 synth as a plugin for Akai MPC standalone hardware: the same engine as the FAKE5 VST3, built with Akai's
MPC Plugin SDK. A poly in the manner of the Sequential Take 5: two oscillators per voice with the Take 5's continuous
SHAPE (sine, sawtooth, square, narrowing pulse), sub, white or pink noise, SYNC 2>1 and FM 2>1, a Prophet-5-style
4-pole low-pass with DRIVE, two DADSR envelopes with ENV ROUTING, two LFOs (global and per voice), sixteen modulation
slots, VINTAGE, OVERDRIVE, twelve effects and the plate REVERB, glide, unison with chord memory, the arpeggiator and
the 64-step sequencer of the 48 factory programs, 5 or 10 voices.

Unofficial: not made, endorsed or supported by Sequential.

## Installieren (kurz auf Deutsch)

1. MPC OS **3.10.1 oder neuer**. Auf der MPC: **Preferences → Enhancements (Beta) → Enable Enhancements** und
   **External Plugin Support** einschalten.
2. Den Ordner **`swtdr34mz - VST - FAKE5`** (aus dem Download `FAKE5-MPC.zip`) auf einen USB-Stick, eine SD-Karte
   oder das interne Laufwerk der MPC kopieren, in einen Ordner namens **`VST2 Plugins`** ganz oben auf dem Laufwerk:
   `VST2 Plugins/swtdr34mz - VST - FAKE5/`.
3. Die MPC neu starten. FAKE5 steht dann im Plugin-Browser unter **External Plugins** (als Plugin-Programm laden).

Die MPC XL, Live III (64 Bit) und die 32-Bit-Geräte (X, X SE, Live, Live II, One, One+, Key 61, Key 37) nehmen
denselben Ordner; jedes Gerät lädt seine eigene Datei darin.

## Die Seiten

Sechs Seiten auf den Funktionstasten; manche haben eine zweite Hälfte: **die Taste nochmal drücken**.

| Taste | Was drauf ist |
| --- | --- |
| **OSC** | Oszillatoren (Octave, Pitch, Shape, Sync 2>1) und Mixer (Osc 1, Sub, Osc 2, FM 2>1, Noise, Vintage, Overdrive, Voice Spread) · nochmal: Glide, Unison, Vintage, Key Follow, Osc 2 Bypass |
| **FILTER** | Filter (Cutoff, Resonance, Drive, Key) und Env 1 · nochmal: Env 2, Env Routing, Repeat, Aux-Ziel |
| **LFO** | LFO 1 (global) und LFO 2 (pro Stimme): Frequenz, Menge, Form, Sync, Ziel, Reset, Slew |
| **MOD** | die Modulations-Slots 1-8 · nochmal: 9-16 (Quelle, Menge, Ziel) |
| **FX** | Effect (Typ, Time, Depth/Mix, Fback/Misc, Clock Sync), Reverb (Mix, Damping, Predelay, Decay, Tone), Overdrive |
| **PLAY** | Arpeggiator, Clock Divide, Sequencer PLAY, Hold; 5/10 Stimmen, Qualität, Program-Lautstärke, Bend, Low Split |

Ein Knopf wird mit dem Finger hoch und runter gezogen oder mit seinem Q-Link gedreht; zweimal tippen öffnet den
großen Regler der MPC (bei einer Auswahl die Liste). Ein Taster schaltet um, eine LED-Liste wählt aus. Die Q-Links
liegen auf jeder Seite auf den Reglern mit Rahmen (obere Reihe der Q-Links = obere Gruppe).

Im **Program-Modus** (ohne Plugin-Seite) machen die Q-Links: 13-16 Cutoff, Resonance, Env 1 Amount, Drive;
9-12 Env 1 (Attack, Decay, Sustain, Release); 5-8 Env 2; 1-4 Reverb Mix, Effect Mix, LFO 1 Menge, Lautstärke.

**QUALITY**: ECO (2x) ist voreingestellt und reicht auf der MPC; HIGH (4x) klingt in den Höhen (und bei FM) noch
etwas sauberer, braucht aber fast doppelt so viel Rechenzeit.

Die Programme: die 48 Werks-Sounds der VST3 und das Basic Program (Programm-Wechsel auf der MPC), mit ihren Sequenzen.
Das Tempo kommt von der MPC.

## Licence

MPL-2.0. Fonts: Fira Sans and Anonymous Pro, SIL Open Font License.
