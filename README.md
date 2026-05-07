<!--
  - SPDX-License-Identifier: CC0-1.0
  - SPDX-FileCopyrightText: 2019 Bhushan Shah <bshah@kde.org>
  - SPDX-FileCopyrightText: 2019-2020 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
 -->

# <img src="keysmith.svg" width="40"/> Keysmith

Keysmith is an application to generate two-factor authentication (2FA) tokens when logging in to your (online) accounts. Currently it supports both HOTP and TOTP tokens.

<a href='https://flathub.org/apps/details/org.kde.keysmith'><img width='190px' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-i-en.png'/></a>

![Keysmith Main Window](https://cdn.kde.org/screenshots/keysmith/keysmith.png)

## Plasma Desktop Applet

This project also provides a KDE Plasma desktop/widget applet (`org.kde.plasma.keysmith`) that brings 2FA tokens directly to your desktop without opening the full Keysmith application.

### Features

- Displays account list with issuer and account name for easy identification
- Shows TOTP/HOTP verification codes with bold, large font for readability
- Click to copy token to clipboard
- 30-second countdown progress bar for TOTP accounts showing time until next refresh
- Auto-unlock via KWallet (no password prompt when wallet is open)

### Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make
sudo make install
```

After installation, restart Plasma Shell (`killall plasmashell && systemctl --user start plasma-plasmashell.service`) and add the "Keysmith" widget to your desktop or panel.

## TODO

 - QR code scanning
 - Backup and Restore of accounts

Originally this code was based largely on the [authenticator-ng](https://github.com/dobey/authenticator-ng) application by Rodney Dawes and Michael Zanetti for Ubuntu Touch.
