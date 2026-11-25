# FAKE5 for the MPC

The FAKE5 synth as a plugin for Akai MPC standalone hardware: the same engine as the FAKE5 VST3, built with Akai's
MPC Plugin SDK. A poly in the manner of the Sequential Take 5: two oscillators per voice with the Take 5's continuous
SHAPE (sine, sawtooth, square, narrowing pulse), sub, white or pink noise, SYNC 2>1 and FM 2>1, a Prophet-5-style
4-pole low-pass with DRIVE, two DADSR envelopes with ENV ROUTING, two LFOs (global and per voice), sixteen modulation
slots, VINTAGE, OVERDRIVE, twelve effects and the plate REVERB, glide, unison with chord memory, the arpeggiator,
64 factory programs, 5 or 10 voices. (The VST's step sequencer is not on the MPC pages: the MPC has its own.)

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
| **OSC** | OSCILLATORS und MIXER wie am Take 5 (Octave mit seinen LEDs, Pitch, Shape, Sync 2>1, Osc 1, Sub, Osc 2, FM 2>1, Noise) · darunter, mittig: LOW SPLIT mit SPLIT POINT, Key Follow 1 und 2, Osc 2 Bypass |
| **FILTER** | FILTER und ENVELOPES wie am Take 5 (Cutoff, Resonance, Drive, beide Hüllkurven, Env Routing) · darunter: Key Amount, Delays, Aux-Ziel (mit Anzeige), Envelope Repeat (ein Taster je Einstellung) |
| **LFO** | LFO 1: GLOBAL und LFO 2: PER VOICE nebeneinander: Frequency, Amount, Shape (ein Regler, die Formen drumherum), Clk Sync, Key Reset, Slew, das Ziel auf einer Anzeige · nochmal: MOD 1-8 auf dem Display · nochmal: MOD 9-16 (antippen und ziehen, doppelt antippen für die Liste) |
| **FX** | EFFECT und REVERB wie am Take 5; über TYPE steht der gewählte Effekt |
| **ARP** | CLOCK (BPM, Clock Divide, mit Anzeigen) · ARPEGGIATOR (On, Relatch, Hold, Oktaven, Wiederholungen) · MODE: ein Taster je Modus |
| **VOICE** | Voice Spread, Vintage · Volume, Overdrive, Glide Rate, Program Vol · Unison (Stimmen, Detune, Key Mode als Taster, Retrigger) und Glide (Osc 1/2 Glide, Modus als Taster) · Glide, Unison, Hold · 5/10 Stimmen, Qualität · Bend |

Ein Knopf wird mit dem Finger hoch und runter gezogen oder mit seinem Q-Link gedreht; zweimal tippen öffnet den
großen Regler der MPC (bei einer Auswahl die Liste). Ein Taster schaltet um; in einer Reihe Taster (je einer pro Einstellung) leuchtet der gewählte. Die Q-Links
liegen auf jeder Seite auf ihren Reglern (obere Reihe der Q-Links = obere Gruppe; auf LFO: LFO 1, LFO 2, dann die beiden Ziele).

Im **Program-Modus** (ohne Plugin-Seite) machen die Q-Links: 13-16 Cutoff, Resonance, Env 1 Amount, Drive;
9-12 Env 1 (Attack, Decay, Sustain, Release); 5-8 Env 2; 1-4 Reverb Mix, Effect Mix, LFO 1 Menge, Lautstärke.

**QUALITY**: ECO (2x) ist voreingestellt und reicht auf der MPC; HIGH (4x) klingt in den Höhen (und bei FM) noch
etwas sauberer, braucht aber fast doppelt so viel Rechenzeit.

Die Programme: die 48 Werks-Sounds der VST3 und das Basic Program (Programm-Wechsel auf der MPC), mit ihren Sequenzen.
Das Tempo kommt von der MPC.

## Licence

MPL-2.0. Fonts: Fira Sans and Anonymous Pro, SIL Open Font License.
