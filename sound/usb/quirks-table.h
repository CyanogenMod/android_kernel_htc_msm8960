/*
 * ALSA USB Audio Driver
 *
 * Copyright (c) 2002 by Takashi Iwai <tiwai@suse.de>,
 *                       Clemens Ladisch <clemens@ladisch.de>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */


#define USB_DEVICE_VENDOR_SPEC(vend, prod) \
	.match_flags = USB_DEVICE_ID_MATCH_VENDOR | \
		       USB_DEVICE_ID_MATCH_PRODUCT | \
		       USB_DEVICE_ID_MATCH_INT_CLASS, \
	.idVendor = vend, \
	.idProduct = prod, \
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC

{
	USB_DEVICE(0x0403, 0xb8d8),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = 0,
		.type = QUIRK_MIDI_FTDI
	}
},

{
	USB_DEVICE(0x041e, 0x3048),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Toshiba",
		.product_name = "SB-0500",
		.ifnum = QUIRK_NO_INTERFACE
	}
},

{
	USB_DEVICE(0x041e, 0x3010),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Creative Labs",
		.product_name = "Sound Blaster MP3+",
		.ifnum = QUIRK_NO_INTERFACE
	}
},
{
	
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE,
	.idVendor = 0x041e,
	.idProduct = 0x3f02,
	.bInterfaceClass = USB_CLASS_AUDIO,
},
{
	
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE,
	.idVendor = 0x041e,
	.idProduct = 0x3f04,
	.bInterfaceClass = USB_CLASS_AUDIO,
},
{
	
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE,
	.idVendor = 0x041e,
	.idProduct = 0x3f0a,
	.bInterfaceClass = USB_CLASS_AUDIO,
},
{
	
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE,
	.idVendor = 0x041e,
	.idProduct = 0x3f19,
	.bInterfaceClass = USB_CLASS_AUDIO,
},

{
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.idVendor = 0x046d,
	.idProduct = 0x0850,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL
},
{
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.idVendor = 0x046d,
	.idProduct = 0x08ae,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL
},
{
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.idVendor = 0x046d,
	.idProduct = 0x08c6,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL
},
{
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.idVendor = 0x046d,
	.idProduct = 0x08f0,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL
},
{
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.idVendor = 0x046d,
	.idProduct = 0x08f5,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL
},
{
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.idVendor = 0x046d,
	.idProduct = 0x08f6,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL
},
{
	USB_DEVICE(0x046d, 0x0990),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Logitech, Inc.",
		.product_name = "QuickCam Pro 9000",
		.ifnum = QUIRK_NO_INTERFACE
	}
},


#define YAMAHA_DEVICE(id, name) { \
	USB_DEVICE(0x0499, id), \
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) { \
		.vendor_name = "Yamaha", \
		.product_name = name, \
		.ifnum = QUIRK_ANY_INTERFACE, \
		.type = QUIRK_MIDI_YAMAHA \
	} \
}
#define YAMAHA_INTERFACE(id, intf, name) { \
	USB_DEVICE_VENDOR_SPEC(0x0499, id), \
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) { \
		.vendor_name = "Yamaha", \
		.product_name = name, \
		.ifnum = intf, \
		.type = QUIRK_MIDI_YAMAHA \
	} \
}
YAMAHA_DEVICE(0x1000, "UX256"),
YAMAHA_DEVICE(0x1001, "MU1000"),
YAMAHA_DEVICE(0x1002, "MU2000"),
YAMAHA_DEVICE(0x1003, "MU500"),
YAMAHA_INTERFACE(0x1004, 3, "UW500"),
YAMAHA_DEVICE(0x1005, "MOTIF6"),
YAMAHA_DEVICE(0x1006, "MOTIF7"),
YAMAHA_DEVICE(0x1007, "MOTIF8"),
YAMAHA_DEVICE(0x1008, "UX96"),
YAMAHA_DEVICE(0x1009, "UX16"),
YAMAHA_INTERFACE(0x100a, 3, "EOS BX"),
YAMAHA_DEVICE(0x100c, "UC-MX"),
YAMAHA_DEVICE(0x100d, "UC-KX"),
YAMAHA_DEVICE(0x100e, "S08"),
YAMAHA_DEVICE(0x100f, "CLP-150"),
YAMAHA_DEVICE(0x1010, "CLP-170"),
YAMAHA_DEVICE(0x1011, "P-250"),
YAMAHA_DEVICE(0x1012, "TYROS"),
YAMAHA_DEVICE(0x1013, "PF-500"),
YAMAHA_DEVICE(0x1014, "S90"),
YAMAHA_DEVICE(0x1015, "MOTIF-R"),
YAMAHA_DEVICE(0x1016, "MDP-5"),
YAMAHA_DEVICE(0x1017, "CVP-204"),
YAMAHA_DEVICE(0x1018, "CVP-206"),
YAMAHA_DEVICE(0x1019, "CVP-208"),
YAMAHA_DEVICE(0x101a, "CVP-210"),
YAMAHA_DEVICE(0x101b, "PSR-1100"),
YAMAHA_DEVICE(0x101c, "PSR-2100"),
YAMAHA_DEVICE(0x101d, "CLP-175"),
YAMAHA_DEVICE(0x101e, "PSR-K1"),
YAMAHA_DEVICE(0x101f, "EZ-J24"),
YAMAHA_DEVICE(0x1020, "EZ-250i"),
YAMAHA_DEVICE(0x1021, "MOTIF ES 6"),
YAMAHA_DEVICE(0x1022, "MOTIF ES 7"),
YAMAHA_DEVICE(0x1023, "MOTIF ES 8"),
YAMAHA_DEVICE(0x1024, "CVP-301"),
YAMAHA_DEVICE(0x1025, "CVP-303"),
YAMAHA_DEVICE(0x1026, "CVP-305"),
YAMAHA_DEVICE(0x1027, "CVP-307"),
YAMAHA_DEVICE(0x1028, "CVP-309"),
YAMAHA_DEVICE(0x1029, "CVP-309GP"),
YAMAHA_DEVICE(0x102a, "PSR-1500"),
YAMAHA_DEVICE(0x102b, "PSR-3000"),
YAMAHA_DEVICE(0x102e, "ELS-01/01C"),
YAMAHA_DEVICE(0x1030, "PSR-295/293"),
YAMAHA_DEVICE(0x1031, "DGX-205/203"),
YAMAHA_DEVICE(0x1032, "DGX-305"),
YAMAHA_DEVICE(0x1033, "DGX-505"),
YAMAHA_DEVICE(0x1034, NULL),
YAMAHA_DEVICE(0x1035, NULL),
YAMAHA_DEVICE(0x1036, NULL),
YAMAHA_DEVICE(0x1037, NULL),
YAMAHA_DEVICE(0x1038, NULL),
YAMAHA_DEVICE(0x1039, NULL),
YAMAHA_DEVICE(0x103a, NULL),
YAMAHA_DEVICE(0x103b, NULL),
YAMAHA_DEVICE(0x103c, NULL),
YAMAHA_DEVICE(0x103d, NULL),
YAMAHA_DEVICE(0x103e, NULL),
YAMAHA_DEVICE(0x103f, NULL),
YAMAHA_DEVICE(0x1040, NULL),
YAMAHA_DEVICE(0x1041, NULL),
YAMAHA_DEVICE(0x1042, NULL),
YAMAHA_DEVICE(0x1043, NULL),
YAMAHA_DEVICE(0x1044, NULL),
YAMAHA_DEVICE(0x1045, NULL),
YAMAHA_INTERFACE(0x104e, 0, NULL),
YAMAHA_DEVICE(0x104f, NULL),
YAMAHA_DEVICE(0x1050, NULL),
YAMAHA_DEVICE(0x1051, NULL),
YAMAHA_DEVICE(0x1052, NULL),
YAMAHA_INTERFACE(0x1053, 0, NULL),
YAMAHA_INTERFACE(0x1054, 0, NULL),
YAMAHA_DEVICE(0x1055, NULL),
YAMAHA_DEVICE(0x1056, NULL),
YAMAHA_DEVICE(0x1057, NULL),
YAMAHA_DEVICE(0x1058, NULL),
YAMAHA_DEVICE(0x1059, NULL),
YAMAHA_DEVICE(0x105a, NULL),
YAMAHA_DEVICE(0x105b, NULL),
YAMAHA_DEVICE(0x105c, NULL),
YAMAHA_DEVICE(0x105d, NULL),
{
	USB_DEVICE(0x0499, 0x1503),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 3,
				.type = QUIRK_MIDI_YAMAHA
			},
			{
				.ifnum = -1
			}
		}
	}
},
YAMAHA_DEVICE(0x2000, "DGP-7"),
YAMAHA_DEVICE(0x2001, "DGP-5"),
YAMAHA_DEVICE(0x2002, NULL),
YAMAHA_DEVICE(0x2003, NULL),
YAMAHA_DEVICE(0x5000, "CS1D"),
YAMAHA_DEVICE(0x5001, "DSP1D"),
YAMAHA_DEVICE(0x5002, "DME32"),
YAMAHA_DEVICE(0x5003, "DM2000"),
YAMAHA_DEVICE(0x5004, "02R96"),
YAMAHA_DEVICE(0x5005, "ACU16-C"),
YAMAHA_DEVICE(0x5006, "NHB32-C"),
YAMAHA_DEVICE(0x5007, "DM1000"),
YAMAHA_DEVICE(0x5008, "01V96"),
YAMAHA_DEVICE(0x5009, "SPX2000"),
YAMAHA_DEVICE(0x500a, "PM5D"),
YAMAHA_DEVICE(0x500b, "DME64N"),
YAMAHA_DEVICE(0x500c, "DME24N"),
YAMAHA_DEVICE(0x500d, NULL),
YAMAHA_DEVICE(0x500e, NULL),
YAMAHA_DEVICE(0x500f, NULL),
YAMAHA_DEVICE(0x7000, "DTX"),
YAMAHA_DEVICE(0x7010, "UB99"),
#undef YAMAHA_DEVICE
#undef YAMAHA_INTERFACE

{
	USB_DEVICE(0x0582, 0x0000),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "UA-100",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S16_LE,
					.channels = 4,
					.iface = 0,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = 0,
					.endpoint = 0x01,
					.ep_attr = 0x09,
					.rates = SNDRV_PCM_RATE_CONTINUOUS,
					.rate_min = 44100,
					.rate_max = 44100,
				}
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S16_LE,
					.channels = 2,
					.iface = 1,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = UAC_EP_CS_ATTR_FILL_MAX,
					.endpoint = 0x81,
					.ep_attr = 0x05,
					.rates = SNDRV_PCM_RATE_CONTINUOUS,
					.rate_min = 44100,
					.rate_max = 44100,
				}
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0007,
					.in_cables  = 0x0007
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0002),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UM-4",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x000f,
					.in_cables  = 0x000f
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0003),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "SC-8850",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x003f,
					.in_cables  = 0x003f
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0004),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "U-8",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0005,
					.in_cables  = 0x0005
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0005),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UM-2",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0003,
					.in_cables  = 0x0003
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0007),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "SC-8820",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0013,
					.in_cables  = 0x0013
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0008),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "PC-300",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0009),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UM-1",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x000b),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "SK-500",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0013,
					.in_cables  = 0x0013
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x000c),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "SC-D70",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S24_3LE,
					.channels = 2,
					.iface = 0,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = 0,
					.endpoint = 0x01,
					.ep_attr = 0x01,
					.rates = SNDRV_PCM_RATE_CONTINUOUS,
					.rate_min = 44100,
					.rate_max = 44100,
				}
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S24_3LE,
					.channels = 2,
					.iface = 1,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = 0,
					.endpoint = 0x81,
					.ep_attr = 0x01,
					.rates = SNDRV_PCM_RATE_CONTINUOUS,
					.rate_min = 44100,
					.rate_max = 44100,
				}
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0007,
					.in_cables  = 0x0007
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{	
	USB_DEVICE(0x0582, 0x0010),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UA-5",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0012),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "XV-5050",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0014),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UM-880",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x01ff,
			.in_cables  = 0x01ff
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0016),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "SD-90",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x000f,
					.in_cables  = 0x000f
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x001b),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "MMP-2",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x001d),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "V-SYNTH",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0023),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UM-550",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x003f,
			.in_cables  = 0x003f
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0025),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UA-20",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S24_3LE,
					.channels = 2,
					.iface = 1,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = 0,
					.endpoint = 0x01,
					.ep_attr = 0x01,
					.rates = SNDRV_PCM_RATE_CONTINUOUS,
					.rate_min = 44100,
					.rate_max = 44100,
				}
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S24_3LE,
					.channels = 2,
					.iface = 2,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = 0,
					.endpoint = 0x82,
					.ep_attr = 0x01,
					.rates = SNDRV_PCM_RATE_CONTINUOUS,
					.rate_min = 44100,
					.rate_max = 44100,
				}
			},
			{
				.ifnum = 3,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0027),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "SD-20",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0003,
			.in_cables  = 0x0007
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0029),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "SD-80",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x000f,
			.in_cables  = 0x000f
		}
	}
},
{	
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x002b),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UA-700",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = 3,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x002d),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "XV-2020",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x002f),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "VariOS",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0007,
			.in_cables  = 0x0007
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0033),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "PCR",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0003,
			.in_cables  = 0x0007
		}
	}
},
	
{
	USB_DEVICE(0x0582, 0x0037),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "Digital Piano",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x003b),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "BOSS",
		.product_name = "GS-10",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = & (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 3,
				.type = QUIRK_MIDI_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0040),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "GI-20",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0042),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "RS-70",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0047),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0048),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0003,
			.in_cables  = 0x0007
		}
	}
},
	
{
	
	USB_DEVICE(0x0582, 0x004c),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "PCR-A",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x004d),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "PCR-A",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0003,
			.in_cables  = 0x0007
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0050),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UA-3FX",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0052),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UM-1SX",
		.ifnum = 0,
		.type = QUIRK_MIDI_STANDARD_INTERFACE
	}
},
{
	USB_DEVICE(0x0582, 0x0060),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "EXR Series",
		.ifnum = 0,
		.type = QUIRK_MIDI_STANDARD_INTERFACE
	}
},
{
	
	USB_DEVICE(0x0582, 0x0064),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0065),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0003
		}
	}
},
{
	
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x006a),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "SP-606",
		.ifnum = 3,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x006d),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "FANTOM-X",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{	
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x0074),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UA-25",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0075),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "BOSS",
		.product_name = "DR-880",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x007a),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0003,
			.in_cables  = 0x0003
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0080),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "G-70",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
	
	
{
	
	USB_DEVICE(0x0582, 0x008b),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "PC-50",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
	
{
	USB_DEVICE(0x0582, 0x0096),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UA-1EX",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x009a),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UM-3EX",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x000f,
			.in_cables  = 0x000f
		}
	}
},
{
	USB_DEVICE(0x0582, 0x00a3),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UA-4FX",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = -1
			}
		}
	}
},
	
{
	USB_DEVICE(0x582, 0x00a6),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "Juno-G",
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x00ad),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "SH-201",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x00c2),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "SonicCell",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x00c4),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x00da),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x00e6),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "EDIROL",
		.product_name = "UA-25EX",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_EDIROL_UAXX
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x00e9),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x0104),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x0108),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.ifnum = 0,
		.type = QUIRK_MIDI_STANDARD_INTERFACE
	}
},
{
	
	USB_DEVICE(0x0582, 0x0109),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE_VENDOR_SPEC(0x0582, 0x010f),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = 1,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0003,
			.in_cables  = 0x0007
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x0111),
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Roland",
		.product_name = "GAIA",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = &(const struct snd_usb_midi_endpoint_info) {
				.out_cables = 0x0003,
				.in_cables  = 0x0003
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0113),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0127),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_STANDARD_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	
	USB_DEVICE(0x0582, 0x012a),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = 0,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0003
		}
	}
},
{
	USB_DEVICE(0x0582, 0x011e),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0582, 0x0130),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 3,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},

{
	USB_DEVICE_VENDOR_SPEC(0x06f8, 0xb000),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Hercules",
		.product_name = "DJ Console (WE)",
		.ifnum = 4,
		.type = QUIRK_MIDI_FIXED_ENDPOINT,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables = 0x0001
		}
	}
},

{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x1002),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "MidiSport 2x2",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_MIDI_MIDIMAN,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0003,
			.in_cables  = 0x0003
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x1011),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "MidiSport 1x1",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_MIDI_MIDIMAN,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x1015),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "Keystation",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_MIDI_MIDIMAN,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x1021),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "MidiSport 4x4",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_MIDI_MIDIMAN,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x000f,
			.in_cables  = 0x000f
		}
	}
},
{
	USB_DEVICE_VER(0x0763, 0x1031, 0x0100, 0x0109),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "MidiSport 8x8",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_MIDI_MIDIMAN,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x01ff,
			.in_cables  = 0x01ff
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x1033),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "MidiSport 8x8",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_MIDI_MIDIMAN,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x01ff,
			.in_cables  = 0x01ff
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x1041),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "MidiSport 2x4",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_MIDI_MIDIMAN,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x000f,
			.in_cables  = 0x0003
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x2001),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "Quattro",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = & (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 3,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 4,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 5,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 6,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 7,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 8,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 9,
				.type = QUIRK_MIDI_MIDIMAN,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x2003),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "AudioPhile",
		.ifnum = 6,
		.type = QUIRK_MIDI_MIDIMAN,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x2008),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "Ozone",
		.ifnum = 3,
		.type = QUIRK_MIDI_MIDIMAN,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x200d),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "M-Audio",
		.product_name = "OmniStudio",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = & (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 3,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 4,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 5,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 6,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = 7,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 8,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 9,
				.type = QUIRK_MIDI_MIDIMAN,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE(0x0763, 0x2019),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = & (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_STANDARD_INTERFACE
			},
			{
				.ifnum = 3,
				.type = QUIRK_MIDI_MIDIMAN,
				.data = & (const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0001,
					.in_cables  = 0x0001
				}
			},
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x2080),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = & (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_MIXER,
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S24_3LE,
					.channels = 8,
					.iface = 1,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = UAC_EP_CS_ATTR_SAMPLE_RATE,
					.endpoint = 0x01,
					.ep_attr = 0x09,
					.rates = SNDRV_PCM_RATE_44100 |
						 SNDRV_PCM_RATE_48000 |
						 SNDRV_PCM_RATE_88200 |
						 SNDRV_PCM_RATE_96000,
					.rate_min = 44100,
					.rate_max = 96000,
					.nr_rates = 4,
					.rate_table = (unsigned int[]) {
						44100, 48000, 88200, 96000
					}
				}
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S24_3LE,
					.channels = 8,
					.iface = 2,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = UAC_EP_CS_ATTR_SAMPLE_RATE,
					.endpoint = 0x81,
					.ep_attr = 0x05,
					.rates = SNDRV_PCM_RATE_44100 |
						 SNDRV_PCM_RATE_48000 |
						 SNDRV_PCM_RATE_88200 |
						 SNDRV_PCM_RATE_96000,
					.rate_min = 44100,
					.rate_max = 96000,
					.nr_rates = 4,
					.rate_table = (unsigned int[]) {
						44100, 48000, 88200, 96000
					}
				}
			},
			
			{
				.ifnum = -1
			}
		}
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0763, 0x2081),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = & (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_AUDIO_STANDARD_MIXER,
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S24_3LE,
					.channels = 8,
					.iface = 1,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = UAC_EP_CS_ATTR_SAMPLE_RATE,
					.endpoint = 0x01,
					.ep_attr = 0x09,
					.rates = SNDRV_PCM_RATE_44100 |
						 SNDRV_PCM_RATE_48000 |
						 SNDRV_PCM_RATE_88200 |
						 SNDRV_PCM_RATE_96000,
					.rate_min = 44100,
					.rate_max = 96000,
					.nr_rates = 4,
					.rate_table = (unsigned int[]) {
							44100, 48000, 88200, 96000
					}
				}
			},
			{
				.ifnum = 2,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = & (const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S24_3LE,
					.channels = 8,
					.iface = 2,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = UAC_EP_CS_ATTR_SAMPLE_RATE,
					.endpoint = 0x81,
					.ep_attr = 0x05,
					.rates = SNDRV_PCM_RATE_44100 |
						 SNDRV_PCM_RATE_48000 |
						 SNDRV_PCM_RATE_88200 |
						 SNDRV_PCM_RATE_96000,
					.rate_min = 44100,
					.rate_max = 96000,
					.nr_rates = 4,
					.rate_table = (unsigned int[]) {
						44100, 48000, 88200, 96000
					}
				}
			},
			
			{
				.ifnum = -1
			}
		}
	}
},

{
	USB_DEVICE(0x07cf, 0x6801),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Casio",
		.product_name = "PL-40R",
		.ifnum = 0,
		.type = QUIRK_MIDI_YAMAHA
	}
},
{
	
	USB_DEVICE(0x07cf, 0x6802),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Casio",
		.product_name = "Keyboard",
		.ifnum = 0,
		.type = QUIRK_MIDI_YAMAHA
	}
},

{
	
	.match_flags = USB_DEVICE_ID_MATCH_VENDOR |
		       USB_DEVICE_ID_MATCH_PRODUCT |
		       USB_DEVICE_ID_MATCH_DEV_SUBCLASS,
	.idVendor = 0x07fd,
	.idProduct = 0x0001,
	.bDeviceSubClass = 2,
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "MOTU",
		.product_name = "Fastlane",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = & (const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 0,
				.type = QUIRK_MIDI_RAW_BYTES
			},
			{
				.ifnum = 1,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},

{
	USB_DEVICE(0x086a, 0x0001),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Emagic",
		
		.ifnum = 2,
		.type = QUIRK_MIDI_EMAGIC,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x80ff,
			.in_cables  = 0x80ff
		}
	}
},
{
	USB_DEVICE(0x086a, 0x0002),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Emagic",
		
		.ifnum = 2,
		.type = QUIRK_MIDI_EMAGIC,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x80ff,
			.in_cables  = 0x80ff
		}
	}
},
{
	USB_DEVICE(0x086a, 0x0003),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Emagic",
		
		.ifnum = 2,
		.type = QUIRK_MIDI_EMAGIC,
		.data = & (const struct snd_usb_midi_endpoint_info) {
			.out_cables = 0x800f,
			.in_cables  = 0x8003
		}
	}
},

{
	USB_DEVICE_VENDOR_SPEC(0x0944, 0x0200),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "KORG, Inc.",
		
		.ifnum = 3,
		.type = QUIRK_MIDI_STANDARD_INTERFACE,
	}
},

{
	USB_DEVICE_VENDOR_SPEC(0x0944, 0x0201),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "KORG, Inc.",
		
		.ifnum = 3,
		.type = QUIRK_MIDI_STANDARD_INTERFACE,
	}
},

{
	USB_DEVICE(0x09e8, 0x0062),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "AKAI",
		.product_name = "MPD16",
		.ifnum = 0,
		.type = QUIRK_MIDI_AKAI,
	}
},

{
	USB_DEVICE_VENDOR_SPEC(0x0ccd, 0x0012),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "TerraTec",
		.product_name = "PHASE 26",
		.ifnum = 3,
		.type = QUIRK_MIDI_STANDARD_INTERFACE
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0ccd, 0x0013),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "TerraTec",
		.product_name = "PHASE 26",
		.ifnum = 3,
		.type = QUIRK_MIDI_STANDARD_INTERFACE
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0ccd, 0x0014),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "TerraTec",
		.product_name = "PHASE 26",
		.ifnum = 3,
		.type = QUIRK_MIDI_STANDARD_INTERFACE
	}
},
{
	USB_DEVICE(0x0ccd, 0x0028),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "TerraTec",
		.product_name = "Aureon5.1MkII",
		.ifnum = QUIRK_NO_INTERFACE
	}
},
{
	USB_DEVICE(0x0ccd, 0x0035),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Miditech",
		.product_name = "Play'n Roll",
		.ifnum = 0,
		.type = QUIRK_MIDI_CME
	}
},

{
	USB_DEVICE(0x103d, 0x0100),
		.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Stanton",
		.product_name = "ScratchAmp",
		.ifnum = QUIRK_NO_INTERFACE
	}
},
{
	USB_DEVICE(0x103d, 0x0101),
		.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Stanton",
		.product_name = "ScratchAmp",
		.ifnum = QUIRK_NO_INTERFACE
	}
},

{
	USB_DEVICE_VENDOR_SPEC(0x1235, 0x0001),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Novation",
		.product_name = "ReMOTE Audio/XStation",
		.ifnum = 4,
		.type = QUIRK_MIDI_NOVATION
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x1235, 0x0002),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Novation",
		.product_name = "Speedio",
		.ifnum = 3,
		.type = QUIRK_MIDI_NOVATION
	}
},
{
	USB_DEVICE(0x1235, 0x000e),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		
		
		.ifnum = 0,
		.type = QUIRK_MIDI_RAW_BYTES
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x1235, 0x4661),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Novation",
		.product_name = "ReMOTE25",
		.ifnum = 0,
		.type = QUIRK_MIDI_NOVATION
	}
},

{
	
	USB_DEVICE_VENDOR_SPEC(0x133e, 0x0815),
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = &(const struct snd_usb_audio_quirk[]) {
			{
				.ifnum = 3,
				.type = QUIRK_MIDI_FIXED_ENDPOINT,
				.data = &(const struct snd_usb_midi_endpoint_info) {
					.out_cables = 0x0003,
					.in_cables  = 0x0003
				}
			},
			{
				.ifnum = 4,
				.type = QUIRK_IGNORE_INTERFACE
			},
			{
				.ifnum = -1
			}
		}
	}
},

{
	
	USB_DEVICE(0x13e5, 0x0001),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Rane",
		.product_name = "SL-1",
		.ifnum = QUIRK_NO_INTERFACE
	}
},

{
	
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE,
	.idVendor = 0x17cc,
	.idProduct = 0x1000,
},
{
	
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE,
	.idVendor = 0x17cc,
	.idProduct = 0x1010,
},
{
	
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE,
	.idVendor = 0x17cc,
	.idProduct = 0x1020,
},

{
	USB_DEVICE(0x1f38, 0x0001),
	.bInterfaceClass = USB_CLASS_AUDIO,
},

{
	USB_DEVICE(0x4752, 0x0011),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.vendor_name = "Miditech",
		.product_name = "Midistart-2",
		.ifnum = 0,
		.type = QUIRK_MIDI_CME
	}
},

{
	
	USB_DEVICE(0x7104, 0x2202),
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.ifnum = 0,
		.type = QUIRK_MIDI_CME
	}
},

{
	USB_DEVICE_VENDOR_SPEC(0x2040, 0x7200),
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Hauppauge",
		.product_name = "HVR-950Q",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_AUDIO_ALIGN_TRANSFER,
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x2040, 0x7240),
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Hauppauge",
		.product_name = "HVR-850",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_AUDIO_ALIGN_TRANSFER,
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x2040, 0x7210),
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Hauppauge",
		.product_name = "HVR-950Q",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_AUDIO_ALIGN_TRANSFER,
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x2040, 0x7217),
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Hauppauge",
		.product_name = "HVR-950Q",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_AUDIO_ALIGN_TRANSFER,
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x2040, 0x721b),
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Hauppauge",
		.product_name = "HVR-950Q",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_AUDIO_ALIGN_TRANSFER,
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x2040, 0x721e),
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Hauppauge",
		.product_name = "HVR-950Q",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_AUDIO_ALIGN_TRANSFER,
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x2040, 0x721f),
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Hauppauge",
		.product_name = "HVR-950Q",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_AUDIO_ALIGN_TRANSFER,
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x2040, 0x7280),
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Hauppauge",
		.product_name = "HVR-950Q",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_AUDIO_ALIGN_TRANSFER,
	}
},
{
	USB_DEVICE_VENDOR_SPEC(0x0fd9, 0x0008),
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Hauppauge",
		.product_name = "HVR-950Q",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_AUDIO_ALIGN_TRANSFER,
	}
},

{
	
	USB_DEVICE(0x0dba, 0x1000),
	.driver_info = (unsigned long) &(const struct snd_usb_audio_quirk) {
		.vendor_name = "Digidesign",
		.product_name = "MBox",
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_COMPOSITE,
		.data = (const struct snd_usb_audio_quirk[]){
			{
				.ifnum = 0,
				.type = QUIRK_IGNORE_INTERFACE,
			},
			{
				.ifnum = 1,
				.type = QUIRK_AUDIO_FIXED_ENDPOINT,
				.data = &(const struct audioformat) {
					.formats = SNDRV_PCM_FMTBIT_S24_3BE,
					.channels = 2,
					.iface = 1,
					.altsetting = 1,
					.altset_idx = 1,
					.attributes = UAC_EP_CS_ATTR_SAMPLE_RATE,
					.endpoint = 0x02,
					.ep_attr = 0x01,
					.maxpacksize = 0x130,
					.rates = SNDRV_PCM_RATE_44100 |
						 SNDRV_PCM_RATE_48000,
					.rate_min = 44100,
					.rate_max = 48000,
					.nr_rates = 2,
					.rate_table = (unsigned int[]) {
						44100, 48000
					}
				}
			},
			{
				.ifnum = -1
			}
		}

	}
},

{
	.match_flags = USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_MIDISTREAMING,
	.driver_info = (unsigned long) & (const struct snd_usb_audio_quirk) {
		.ifnum = QUIRK_ANY_INTERFACE,
		.type = QUIRK_MIDI_STANDARD_INTERFACE
	}
},

#undef USB_DEVICE_VENDOR_SPEC
