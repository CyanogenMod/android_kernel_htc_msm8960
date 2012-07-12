CFLAGS_scm.o :=$(call as-instr,.arch_extension sec,-DREQUIRES_SEC=1)

obj-y += io.o dma.o memory.o
ifndef CONFIG_ARM_ARCH_TIMER
obj-y += timer.o
endif
obj-y += clock.o clock-voter.o clock-dummy.o
obj-y += modem_notifier.o subsystem_map.o
obj-$(CONFIG_CPU_FREQ_MSM) += cpufreq.o
obj-$(CONFIG_DEBUG_FS) += nohlt.o clock-debug.o
obj-$(CONFIG_KEXEC) += msm_kexec.o
obj-y += pwrtest.o
obj-$(CONFIG_MSM_PROC_COMM) += proc_comm.o
ifndef CONFIG_ARCH_MSM8X60
	obj-$(CONFIG_MSM_PROC_COMM) += clock-pcom.o
	obj-$(CONFIG_MSM_PROC_COMM) += vreg.o mpp.o
	ifdef CONFIG_MSM_PROC_COMM
ifndef CONFIG_ARCH_FSM9XXX
		obj-$(CONFIG_REGULATOR) += footswitch-pcom.o
endif
		obj-$(CONFIG_DEBUG_FS) += pmic_debugfs.o
	endif
endif

obj-y += acpuclock.o
obj-y += radio_feedback.o
obj-$(CONFIG_ARCH_MSM7X01A) += acpuclock-7201.o
obj-$(CONFIG_ARCH_MSM7X25) += acpuclock-7201.o
obj-$(CONFIG_ARCH_MSM7X27) += acpuclock-7201.o
obj-$(CONFIG_ARCH_MSM_SCORPION) += pmu.o
obj-$(CONFIG_ARCH_MSM_KRAIT) += msm-krait-l2-accessors.o pmu.o
obj-$(CONFIG_ARCH_MSM7X27A) += pmu.o

ifndef CONFIG_MSM_SMP
obj-$(CONFIG_ARCH_MSM_SCORPION) += msm_fault_handlers.o
endif

obj-$(CONFIG_MSM_VIC) += irq-vic.o

ifdef CONFIG_ARCH_QSD8X50
	obj-$(CONFIG_MSM_SOC_REV_NONE) += acpuclock-8x50.o
endif

obj-$(CONFIG_SMP) += headsmp.o platsmp.o
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o

obj-$(CONFIG_MSM_CPU_AVS) += avs.o
obj-$(CONFIG_MSM_AVS_HW) += avs_hw.o
obj-$(CONFIG_CPU_V6) += idle-v6.o
obj-$(CONFIG_CPU_V7) += idle-v7.o
obj-$(CONFIG_MSM_JTAG) += jtag.o

msm-etm-objs := etm.o
obj-$(CONFIG_MSM_ETM) += msm-etm.o
obj-$(CONFIG_MSM_QDSS) += qdss.o qdss-etb.o qdss-tpiu.o qdss-funnel.o qdss-ptm.o

ifdef CONFIG_MSM_NONSMD_PACKET_FILTER
obj-$(CONFIG_MSM_SMD) += htc_port_list_lte.o
else
obj-$(CONFIG_MSM_SMD) += htc_port_list.o
endif

quiet_cmd_mkrpcsym = MKCAP   $@
      cmd_mkrpcsym = $(PERL) $(srctree)/$(src)/mkrpcsym.pl $< $@

target += smd_rpc_sym.c
$(obj)/smd_rpc_sym.c: $(src)/smd_rpc_sym $(src)/mkrpcsym.pl
	$(call if_changed,mkrpcsym)

obj-$(CONFIG_MSM_SCM) += scm.o scm-boot.o htc_simlock-8x60.o htc_drm.o htc_sdservice.o
ifdef CONFIG_ARCH_MSM8960
	obj-y += tz_log.o scm-memchk.o
endif
ifdef CONFIG_ARCH_MSM8X60
	obj-y += scm-log.o
endif
obj-$(CONFIG_MSM_SECURE_IO) += scm-io.o
obj-$(CONFIG_MSM_PIL) += peripheral-loader.o
obj-$(CONFIG_MSM_PIL) += scm-pas.o
ifdef CONFIG_MSM_PIL
obj-$(CONFIG_ARCH_MSM8X60) += peripheral-reset.o
obj-$(CONFIG_ARCH_MSM8960) += peripheral-reset-8960.o
endif
obj-$(CONFIG_MSM_PIL_QDSP6V4) += pil-q6v4.o
obj-$(CONFIG_MSM_PIL_RIVA) += pil-riva.o
obj-$(CONFIG_MSM_PIL_TZAPPS) += pil-tzapps.o
obj-$(CONFIG_ARCH_QSD8X50) += sirc.o
obj-$(CONFIG_ARCH_FSM9XXX) += sirc-fsm9xxx.o
obj-$(CONFIG_MSM_FIQ_SUPPORT) += fiq_glue.o
obj-$(CONFIG_MACH_TROUT) += board-trout-rfkill.o
obj-$(CONFIG_MSM_BAM_DMUX) += bam_dmux.o
obj-$(CONFIG_MSM_SMD_LOGGING) += smem_log.o
obj-$(CONFIG_MSM_SMD) += smd.o smd_debug.o remote_spinlock.o
obj-y += socinfo.o
ifndef CONFIG_ARCH_MSM9615
ifndef CONFIG_ARCH_APQ8064
ifndef CONFIG_ARCH_MSM8960
ifndef CONFIG_ARCH_MSM8X60
	obj-$(CONFIG_MSM_SMD) += pmic.o
	obj-$(CONFIG_MSM_ONCRPCROUTER) += rpc_hsusb.o rpc_pmapp.o rpc_fsusb.o
endif
endif
endif
endif
ifndef CONFIG_ARCH_MSM8960
ifndef CONFIG_ARCH_MSM8X60
ifndef CONFIG_ARCH_MSM7X30
ifndef CONFIG_ARCH_APQ8064
ifndef CONFIG_ARCH_MSMCOPPER
	obj-y += nand_partitions.o
endif
endif
endif
endif
endif
ifdef CONFIG_ARCH_MSM8960
	obj-y += emmc_partitions.o
endif
obj-$(CONFIG_MSM_SDIO_TTY) += sdio_tty.o sdio_tty_ciq.o
obj-$(CONFIG_MSM_SMD_TTY) += smd_tty.o
obj-$(CONFIG_MSM_SMD_QMI) += smd_qmi.o
obj-$(CONFIG_MSM_SMD_PKT) += smd_pkt.o
obj-$(CONFIG_MSM_DSPS) += msm_dsps.o
obj-$(CONFIG_MSM_SMD_NMEA) += smd_nmea.o
obj-$(CONFIG_MSM_RESET_MODEM) += reset_modem.o
obj-$(CONFIG_MSM_IPC_ROUTER_SMD_XPRT) += ipc_router_smd_xprt.o
obj-$(CONFIG_MSM_ONCRPCROUTER) += smd_rpcrouter.o
obj-$(CONFIG_MSM_ONCRPCROUTER) += smd_rpcrouter_device.o
obj-$(CONFIG_MSM_IPC_ROUTER) += ipc_router.o
obj-$(CONFIG_MSM_IPC_ROUTER)+= ipc_socket.o
obj-$(CONFIG_DEBUG_FS) += smd_rpc_sym.o
obj-$(CONFIG_MSM_ONCRPCROUTER) += smd_rpcrouter_servers.o
obj-$(CONFIG_MSM_ONCRPCROUTER) += smd_rpcrouter_clients.o
obj-$(CONFIG_MSM_ONCRPCROUTER) += smd_rpcrouter_xdr.o
obj-$(CONFIG_MSM_ONCRPCROUTER) += rpcrouter_smd_xprt.o
obj-$(CONFIG_MSM_RPC_SDIO_XPRT) += rpcrouter_sdio_xprt.o
obj-$(CONFIG_MSM_RPC_PING) += ping_mdm_rpc_client.o
obj-$(CONFIG_MSM_RPC_PROC_COMM_TEST) += proc_comm_test.o
obj-$(CONFIG_MSM_RPC_PING) += ping_mdm_rpc_client.o ping_apps_server.o
obj-$(CONFIG_MSM_RPC_OEM_RAPI) += oem_rapi_client.o
obj-$(CONFIG_MSM_RPC_WATCHDOG) += rpc_dog_keepalive.o
obj-$(CONFIG_MSM_RPCSERVER_WATCHDOG) += rpc_server_dog_keepalive.o
obj-$(CONFIG_MSM_RPCSERVER_TIME_REMOTE) += rpc_server_time_remote.o
obj-$(CONFIG_MSM_DALRPC) += dal.o
obj-$(CONFIG_MSM_DALRPC_TEST) += dal_remotetest.o
obj-$(CONFIG_ARCH_MSM7X30) += dal_axi.o
obj-$(CONFIG_ARCH_MSM7X27A) += dal_axi.o
obj-$(CONFIG_MSM_ADSP) += qdsp5/
#obj-$(CONFIG_MSM7KV2_AUDIO) += qdsp5v2/
obj-$(CONFIG_MSM_RPCSERVER_HANDSET) += rpc_server_handset.o
obj-$(CONFIG_MSM_QDSP6) += qdsp6/
obj-$(CONFIG_MSM8X60_AUDIO) += qdsp6v2_1x/
obj-$(CONFIG_MSM_AUDIO_QDSP6) += qdsp6v2/
obj-$(CONFIG_MSM_HW3D) += hw3d.o
obj-$(CONFIG_PM) += pm-boot.o
obj-$(CONFIG_MSM_PM8X60) += pm-8x60.o
obj-$(CONFIG_MSM_PM2) += pm2.o
obj-$(CONFIG_MSM_PM) += pm.o
obj-$(CONFIG_MSM_NOPM) += no-pm.o

obj-$(CONFIG_MSM_SPM_V1) += spm.o
obj-$(CONFIG_MSM_SPM_V2) += spm-v2.o spm_devices.o

obj-$(CONFIG_MSM_DMA_TEST) += dma_test.o
obj-$(CONFIG_SURF_FFA_GPIO_KEYPAD) += keypad-surf-ffa.o

obj-$(CONFIG_ARCH_MSM7X01A) += board-halibut.o devices-msm7x01a.o clock-pcom-lookup.o
obj-$(CONFIG_MACH_TROUT) += board-trout.o board-trout-gpio.o
obj-$(CONFIG_MACH_TROUT) += board-trout-keypad.o board-trout-panel.o
obj-$(CONFIG_MACH_TROUT) += htc_akm_cal.o htc_wifi_nvs.o htc_acoustic.o
obj-$(CONFIG_MACH_TROUT) += board-trout-mmc.o board-trout-wifi.o
obj-$(CONFIG_ARCH_QSD8X50) += devices-qsd8x50.o clock-pcom-lookup.o
obj-$(CONFIG_MACH_QSD8X50_SURF) += board-qsd8x50.o
obj-$(CONFIG_MACH_QSD8X50_FFA) += board-qsd8x50.o
obj-$(CONFIG_ARCH_MSM8X60) += devices-msm8x60.o clock-local.o clock-8x60.o acpuclock-8x60.o
obj-$(CONFIG_ARCH_MSM8X60) += clock-rpm.o
obj-$(CONFIG_ARCH_MSM8X60) += saw-regulator.o
obj-$(CONFIG_ARCH_MSM8X60) += footswitch-8x60.o
obj-$(CONFIG_MSM8X60_AUDIO) += htc_acoustic_8x60.o
obj-$(CONFIG_ARCH_MSM8960) += htc_acoustic_8960.o
ifdef CONFIG_MSM_SUBSYSTEM_RESTART
	obj-y += subsystem_notif.o
	obj-y += subsystem_restart.o
	obj-y += ramdump.o
	obj-$(CONFIG_ARCH_MSM8X60) += subsystem-fatal-8x60.o
endif
obj-$(CONFIG_MSM_SYSMON_COMM) += sysmon.o
obj-$(CONFIG_MSM_MODEM_8960) += modem-8960.o
obj-$(CONFIG_MSM_LPASS_8960) += lpass-8960.o
obj-$(CONFIG_MSM_WCNSS_SSR_8960) += wcnss-ssr-8960.o

ifdef CONFIG_MSM_RPM_REGULATOR
obj-y += rpm-regulator.o
obj-$(CONFIG_ARCH_MSM8X60) += rpm-regulator-8660.o
obj-$(CONFIG_ARCH_MSM8960) += rpm-regulator-8960.o
obj-$(CONFIG_ARCH_MSM9615) += rpm-regulator-9615.o
endif

ifdef CONFIG_CPU_IDLE
	obj-$(CONFIG_ARCH_MSM8960) += cpuidle.o
	obj-$(CONFIG_ARCH_MSM8X60) += cpuidle.o
	obj-$(CONFIG_ARCH_MSM9615) += cpuidle.o
endif

obj-$(CONFIG_ARCH_FSM9XXX) += devices-fsm9xxx.o
obj-$(CONFIG_ARCH_FSM9XXX) += clock-fsm9xxx.o clock-local.o acpuclock-fsm9xxx.o
obj-$(CONFIG_ARCH_FSM9XXX) += dfe-fsm9xxx.o rfic-fsm9xxx.o
obj-$(CONFIG_ARCH_FSM9XXX) += restart-fsm9xxx.o xo-fsm9xxx.o

obj-$(CONFIG_MSM_WATCHDOG) += msm_watchdog.o
obj-$(CONFIG_MACH_MSM8X60_RUMI3) += board-msm8x60.o
obj-$(CONFIG_MACH_MSM8X60_SIM) += board-msm8x60.o
obj-$(CONFIG_MACH_MSM8X60_SURF) += board-msm8x60.o
obj-$(CONFIG_MACH_MSM8X60_FFA) += board-msm8x60.o
obj-$(CONFIG_MACH_MSM8X60_FLUID) += board-msm8x60.o
obj-$(CONFIG_MACH_MSM8X60_DRAGON) += board-msm8x60.o
obj-$(CONFIG_MACH_TYPE_MSM8X60_FUSION) += board-msm8x60.o mdm.o
obj-$(CONFIG_MACH_MSM8X60_FUSN_FFA) += board-msm8x60.o mdm.o
obj-$(CONFIG_TROUT_H2W) += board-trout-h2w.o
obj-$(CONFIG_TROUT_BATTCHG) += htc_battery.o
obj-$(CONFIG_TROUT_PWRSINK) += htc_pwrsink.o
obj-$(CONFIG_ARCH_MSM7X27) += clock-pcom-lookup.o
obj-$(CONFIG_MACH_MSM7X27_SURF) += board-msm7x27.o devices-msm7x27.o
obj-$(CONFIG_MACH_MSM7X27_FFA) += board-msm7x27.o devices-msm7x27.o
obj-$(CONFIG_ARCH_MSM7X27A) += clock-pcom-lookup.o devices-msm7x27a.o
obj-$(CONFIG_MACH_MSM7X27A_RUMI3) += board-msm7x27a.o
obj-$(CONFIG_MACH_MSM7X27A_SURF) += board-msm7x27a.o
obj-$(CONFIG_MACH_MSM7X27A_FFA) += board-msm7x27a.o
obj-$(CONFIG_ARCH_MSM7X30) += devices-msm7x30.o memory_topology.o devices_htc.o emmc_partitions.o
obj-$(CONFIG_ARCH_MSM7X30) += clock-local.o clock-7x30.o acpuclock-7x30.o
obj-$(CONFIG_MACH_MSM7X25_SURF) += board-msm7x27.o devices-msm7x25.o
obj-$(CONFIG_MACH_MSM7X25_FFA) += board-msm7x27.o devices-msm7x25.o
obj-$(CONFIG_ARCH_MSM8960) += clock-local.o clock-dss-8960.o clock-8960.o clock-rpm.o
obj-$(CONFIG_ARCH_MSM8960) += devices-8960.o footswitch-8x60.o
obj-$(CONFIG_ARCH_MSM8960) += acpuclock-8960.o
obj-$(CONFIG_ARCH_MSM8960) += memory_topology.o
obj-$(CONFIG_ARCH_MSM8960) += saw-regulator.o
obj-$(CONFIG_MACH_MSM8960_SIM) += board-8960.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8960_RUMI3) += board-8960.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8960_CDP) += board-8960.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8960_MTP) += board-8960.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8960_FLUID) += board-8960.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8930_CDP) += board-8960.o devices-8960.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8930_MTP) += board-8960.o devices-8960.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8930_FLUID) += board-8960.o devices-8960.o board-8960-regulator.o
obj-$(CONFIG_ARCH_MSM8960) += bms-batterydata.o
obj-$(CONFIG_ARCH_APQ8064) += board-apq8064.o devices-8064.o board-apq8064-regulator.o
obj-$(CONFIG_ARCH_MSM9615) += board-9615.o devices-9615.o board-9615-regulator.o
obj-$(CONFIG_ARCH_MSM9615) += clock-local.o clock-9615.o acpuclock-9615.o clock-rpm.o
obj-$(CONFIG_ARCH_MSMCOPPER) += board-copper.o board-dt.o
obj-$(CONFIG_ARCH_MSM8960) += devices_htc.o htc_wifi_nvs.o htc_awb_cal.o
obj-$(CONFIG_ARCH_MSM8X60) += devices_htc.o htc_wifi_nvs.o sysinfo-8x60.o emmc_partitions.o htc_awb_cal.o

ifeq ($(CONFIG_ARCH_MSM8X60_LTE),y)
obj-$(CONFIG_ARCH_MSM8X60_LTE) += 8x60-lte/
else
obj-$(CONFIG_MSM_SDIO_AL) += sdio_al.o
obj-$(CONFIG_MSM_SDIO_AL) += sdio_al_test.o
obj-$(CONFIG_MSM_SDIO_AL) += sdio_al_dloader.o
obj-$(CONFIG_MSM_SDIO_DMUX) += sdio_dmux.o
obj-$(CONFIG_MSM_SDIO_CMUX) += sdio_cmux.o
obj-$(CONFIG_MSM_SDIO_CTL) += sdio_ctl.o
endif

# MSM8960 boards
obj-$(CONFIG_MACH_TAHITI) += board-tahiti.o board-tahiti-regulator.o board-tahiti-gpio.o
obj-$(CONFIG_MACH_TAHITI) += board-tahiti-keypad.o
obj-$(CONFIG_MACH_TAHITI) += board-tahiti-audio.o
obj-$(CONFIG_MACH_TAHITI) += htc_bdaddress.o htc_util.o
obj-$(CONFIG_MACH_FIGHTER) += board-fighter.o board-fighter-regulator.o board-fighter-gpio.o
obj-$(CONFIG_MACH_FIGHTER) += board-fighter-keypad.o
obj-$(CONFIG_MACH_FIGHTER) += board-fighter-audio.o
obj-$(CONFIG_MACH_FIGHTER) += htc_bdaddress.o htc_util.o
obj-$(CONFIG_MACH_JET) += board-jet.o board-jet-regulator.o board-jet-gpio.o board-jet-audio.o
obj-$(CONFIG_MACH_JET) += board-jet-keypad.o
obj-$(CONFIG_MACH_JET) += htc_bdaddress.o htc_util.o

obj-$(CONFIG_MACH_JEWEL_DD) += board-jel_dd.o board-jel_dd-regulator.o board-jel_dd-gpio.o 
obj-$(CONFIG_MACH_JEWEL_DD) += board-jel_dd-keypad.o
obj-$(CONFIG_MACH_JEWEL_DD) += display/board-jel_dd-panel.o
obj-$(CONFIG_MACH_JEWEL_DD) += board-jel_dd-audio.o
obj-$(CONFIG_MACH_JEWEL_DD) += htc_bdaddress.o htc_util.o
obj-$(CONFIG_MACH_JEWEL_DD) += board-jel_dd-gsm-radio.o

obj-$(CONFIG_MACH_VILLE) += board-ville.o board-ville-regulator.o board-ville-gpio.o
obj-$(CONFIG_MACH_VILLE) += board-ville-keypad.o
obj-$(CONFIG_MACH_VILLE) += board-ville-audio.o
obj-$(CONFIG_MACH_VILLE) += htc_bdaddress.o htc_util.o

obj-$(CONFIG_MACH_VALENTE_WX) += board-valente_wx.o board-valente_wx-regulator.o board-valente_wx-gpio.o
obj-$(CONFIG_MACH_VALENTE_WX) += board-valente_wx-keypad.o
obj-$(CONFIG_MACH_VALENTE_WX) += display/board-valente_wx-panel.o
obj-$(CONFIG_MACH_VALENTE_WX) += board-valente_wx-audio.o
obj-$(CONFIG_MACH_VALENTE_WX) += htc_bdaddress.o htc_util.o

ifeq ($(CONFIG_WIMAX),y)
obj-$(CONFIG_MACH_VALENTE_WX) += board-valente_wx-mmc.o  board-common-wimax.o
endif

obj-$(CONFIG_MACH_SAPPHIRE) += board-sapphire.o board-sapphire-gpio.o
obj-$(CONFIG_MACH_SAPPHIRE) += board-sapphire-keypad.o board-sapphire-panel.o
obj-$(CONFIG_MACH_SAPPHIRE) += board-sapphire-mmc.o board-sapphire-wifi.o
obj-$(CONFIG_MACH_SAPPHIRE) += board-sapphire-rfkill.o msm_vibrator.o

obj-$(CONFIG_MACH_ELITE) += board-elite.o board-elite-regulator.o board-elite-gpio.o
obj-$(CONFIG_MACH_ELITE) += board-elite-keypad.o
obj-$(CONFIG_MACH_ELITE) += board-elite-audio.o
obj-$(CONFIG_MACH_ELITE) += htc_bdaddress.o htc_util.o

obj-$(CONFIG_MACH_KNIGHT) += board-knight.o board-knight-regulator.o board-knight-gpio.o
obj-$(CONFIG_MACH_KNIGHT) += board-knight-keypad.o
obj-$(CONFIG_MACH_KNIGHT) += board-knight-audio.o
obj-$(CONFIG_MACH_KNIGHT) += htc_bdaddress.o htc_util.o

obj-$(CONFIG_MACH_ELITE_ULK) += board-elite_ulk.o board-elite_ulk-regulator.o board-elite_ulk-gpio.o
obj-$(CONFIG_MACH_ELITE_ULK) += board-elite_ulk-keypad.o
obj-$(CONFIG_MACH_ELITE_ULK) += board-elite_ulk-audio.o htc_util.o

# MSM8x60 boards
obj-$(CONFIG_MACH_PYRAMID) += board-pyramid.o
obj-$(CONFIG_MACH_PYRAMID) += board-pyramid-keypad.o
obj-$(CONFIG_MACH_PYRAMID) += board-pyramid-audio.o
obj-$(CONFIG_HTC_BATT_CORE) += htc_battery_core.o
obj-$(CONFIG_HTC_BATT8x60) += htc_battery_8x60.o
obj-$(CONFIG_HTC_BATT_8960) += htc_battery_8960.o htc_battery_cell.o
obj-$(CONFIG_MACH_PYRAMID) += htc_bdaddress.o board-pyramid-rfkill.o
obj-$(CONFIG_MACH_PYRAMID) += board-pyramid-mmc.o board-pyramid-wifi.o
ifeq ($(CONFIG_ION_MSM),y)
obj-$(CONFIG_MACH_PYRAMID) += board-pyramid-ion.o
endif
obj-$(CONFIG_MACH_SHOOTER) += board-shooter.o
obj-$(CONFIG_MACH_SHOOTER) += board-shooter-keypad.o
obj-$(CONFIG_MACH_SHOOTER) += board-shooter-audio.o
obj-$(CONFIG_MACH_SHOOTER) += htc_bdaddress.o board-shooter-rfkill.o
obj-$(CONFIG_MACH_SHOOTER) += board-shooter-mmc.o board-shooter-wifi.o

obj-$(CONFIG_MACH_HOLIDAY) += board-holiday.o mdm.o
obj-$(CONFIG_MACH_HOLIDAY) += board-holiday-audio.o
obj-$(CONFIG_MACH_HOLIDAY) += board-holiday-keypad.o
obj-$(CONFIG_MACH_HOLIDAY) += htc_bdaddress.o htc_sleep_clk.o board-holiday-rfkill.o htc_fmtx_rfkill.o htc_util.o
obj-$(CONFIG_MACH_HOLIDAY) += board-holiday-mmc.o board-holiday-wifi.o
obj-$(CONFIG_MACH_VIGOR) += board-vigor.o mdm.o
obj-$(CONFIG_MACH_VIGOR) += board-vigor-audio.o
obj-$(CONFIG_MACH_VIGOR) += board-vigor-keypad.o
obj-$(CONFIG_MACH_VIGOR) += htc_bdaddress.o htc_sleep_clk.o board-vigor-rfkill.o htc_fmtx_rfkill.o htc_util.o
obj-$(CONFIG_MACH_VIGOR) += board-vigor-mmc.o board-vigor-wifi.o

# MSM7x30 boards
obj-$(CONFIG_MACH_RUNNYMEDE) += board-runnymede.o

CFLAGS_msm_vibrator.o += -Idrivers/staging/android

obj-$(CONFIG_ARCH_FSM9XXX) += board-fsm9xxx.o

obj-$(CONFIG_TROUT_BATTCHG) += htc_battery.o
obj-$(CONFIG_HTC_BATT_CORE) += htc_battery_core.o
obj-$(CONFIG_HTC_BATT8x60) += htc_battery_8x60.o

obj-$(CONFIG_HTC_PWRSINK) += htc_pwrsink.o
obj-$(CONFIG_HTC_HEADSET_MGR) += htc_headset_mgr.o
obj-$(CONFIG_HTC_HEADSET_H2W) += htc_headset_h2w.o
obj-$(CONFIG_HTC_HEADSET_GPIO) += htc_headset_gpio.o
obj-$(CONFIG_HTC_HEADSET_MICROP) += htc_headset_microp.o
obj-$(CONFIG_HTC_HEADSET_PMIC) += htc_headset_pmic.o
obj-$(CONFIG_HTC_HEADSET_MISC) += htc_headset_misc.o
obj-$(CONFIG_HTC_HEADSET_8X60) += htc_headset_8x60.o
ifeq ($(CONFIG_ARCH_MSM7X30),y)
obj-$(CONFIG_MSM_RMT_STORAGE_CLIENT) += rmt_storage_client-7x30.o
else
obj-$(CONFIG_MSM_RMT_STORAGE_CLIENT) += rmt_storage_client.o
endif
obj-$(CONFIG_MSM_SDIO_SMEM) += sdio_smem.o
obj-$(CONFIG_PMIC8058) += pmic8058-gpio.o pmic8058-mpp.o
obj-$(CONFIG_MSM_RPM) += rpm.o rpm_resources.o
obj-$(CONFIG_MSM_MPM) += mpm.o
obj-$(CONFIG_MSM_RPM_STATS_LOG) += rpm_stats.o
obj-$(CONFIG_MSM_RPM_LOG) += rpm_log.o
obj-$(CONFIG_MSM_XO) += msm_xo.o
obj-$(CONFIG_MSM_BUS_SCALING) += msm_bus/
obj-$(CONFIG_MSM_BUSPM_DEV) += msm-buspm-dev.o

obj-$(CONFIG_MSM_IOMMU)		+= iommu.o iommu_dev.o devices-iommu.o iommu_domains.o

ifdef CONFIG_VCM
obj-$(CONFIG_ARCH_MSM8X60) += board-msm8x60-vcm.o
endif

obj-$(CONFIG_ARCH_MSM7X27) += gpiomux-7x27.o gpiomux-v1.o gpiomux.o
obj-$(CONFIG_ARCH_MSM7X30) += gpiomux-7x30.o gpiomux-v1.o gpiomux.o
obj-$(CONFIG_ARCH_QSD8X50) += gpiomux-8x50.o gpiomux-v1.o gpiomux.o
obj-$(CONFIG_ARCH_MSM8X60) += gpiomux-8x60.o gpiomux-v2.o gpiomux.o
obj-$(CONFIG_ARCH_MSM8960) += gpiomux-v2.o gpiomux.o
obj-$(CONFIG_ARCH_APQ8064) += gpiomux-v2.o gpiomux.o
obj-$(CONFIG_ARCH_MSM9615) += gpiomux-v2.o gpiomux.o
obj-$(CONFIG_ARCH_MSMCOPPER) += gpiomux-v2.o gpiomux.o

ifdef CONFIG_FSM9XXX_TLMM
obj-y   += gpio-fsm9xxx.o
else
ifdef CONFIG_MSM_V2_TLMM
obj-y	+= gpio-v2.o
else
obj-y	+= gpio.o
endif
endif

obj-$(CONFIG_MSM_SLEEP_STATS) += msm_rq_stats.o idle_stats.o
obj-$(CONFIG_MSM_SLEEP_STATS_DEVICE) += idle_stats_device.o
obj-$(CONFIG_MSM_SHOW_RESUME_IRQ) += msm_show_resume_irq.o
obj-$(CONFIG_BT_MSM_PINTEST)  += btpintest.o
obj-$(CONFIG_MSM_FAKE_BATTERY) += fish_battery.o
obj-$(CONFIG_MSM_RPC_VIBRATOR) += msm_vibrator.o
obj-$(CONFIG_MSM_NATIVE_RESTART) += restart.o

obj-$(CONFIG_MSM_PROC_COMM_REGULATOR) += proccomm-regulator.o
ifdef CONFIG_MSM_PROC_COMM_REGULATOR
obj-$(CONFIG_ARCH_MSM7X30) += board-msm7x30-regulator.o
obj-$(CONFIG_ARCH_MSM7X27A) += board-msm7x27a-regulator.o
endif
obj-$(CONFIG_ARCH_MSM8960) += mdm2.o mdm_common.o
obj-$(CONFIG_MSM_RTB) += msm_rtb.o
obj-$(CONFIG_PERFLOCK) += perflock.o
obj-y += display/
