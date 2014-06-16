CFLAGS_scm.o :=$(call as-instr,.arch_extension sec,-DREQUIRES_SEC=1)

obj-y += io.o dma.o memory.o emmc_partitions.o
ifndef CONFIG_ARM_ARCH_TIMER
obj-y += timer.o
endif
obj-y += clock.o clock-voter.o clock-dummy.o
obj-y += modem_notifier.o subsystem_map.o
obj-$(CONFIG_CPU_FREQ_MSM) += cpufreq.o
obj-$(CONFIG_DEBUG_FS) += nohlt.o clock-debug.o
obj-$(CONFIG_KEXEC) += msm_kexec.o

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
obj-$(CONFIG_ARCH_MSM7X27) += acpuclock-7627.o clock-pll.o
obj-$(CONFIG_ARCH_MSM_SCORPION) += pmu.o
obj-$(CONFIG_ARCH_MSM_SCORPIONMP) += perf_event_msm_l2.o
obj-$(CONFIG_ARCH_MSM_KRAIT) += msm-krait-l2-accessors.o pmu.o perf_event_msm_krait_l2.o
obj-$(CONFIG_ARCH_MSM_KRAIT) += krait-scm.o
obj-$(CONFIG_ARCH_MSM7X27A) += pmu.o

ifndef CONFIG_MSM_SMP
obj-$(CONFIG_ARCH_MSM_SCORPION) += msm_fault_handlers.o
endif

obj-$(CONFIG_MSM_VIC) += irq-vic.o

ifdef CONFIG_ARCH_QSD8X50
	obj-$(CONFIG_MSM_SOC_REV_NONE) += acpuclock-8x50.o
endif

obj-$(CONFIG_SMP) += headsmp.o
ifdef CONFIG_ARCH_MSM8625
	obj-$(CONFIG_SMP) += platsmp-8625.o
else
	obj-$(CONFIG_SMP) += platsmp.o
endif
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o

obj-$(CONFIG_MSM_CPU_AVS) += avs.o
obj-$(CONFIG_MSM_AVS_HW) += avs_hw.o
obj-$(CONFIG_CPU_V6) += idle-v6.o
obj-$(CONFIG_CPU_V7) += idle-v7.o
obj-$(CONFIG_MSM_JTAG) += jtag.o

msm-etm-objs := etm.o
obj-$(CONFIG_MSM_ETM) += msm-etm.o

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

obj-$(CONFIG_MSM_SCM) += scm.o scm-boot.o htc_simlock.o htc_sdservice.o htc_drm.o htc_debug.o htc_rmtmsg.o htc_fingerprint.o
obj-$(CONFIG_MSM_SECURE_IO) += scm-io.o
obj-$(CONFIG_MSM_PIL) += peripheral-loader.o
obj-$(CONFIG_MSM_PIL) += scm-pas.o
obj-$(CONFIG_MSM_PIL_QDSP6V3) += pil-q6v3.o
obj-$(CONFIG_MSM_PIL_QDSP6V4) += pil-q6v4.o
obj-$(CONFIG_MSM_PIL_LPASS_QDSP6V5) += pil-q6v5.o pil-q6v5-lpass.o
obj-$(CONFIG_MSM_PIL_MSS_QDSP6V5) += pil-q6v5.o pil-q6v5-mss.o
obj-$(CONFIG_MSM_PIL_MBA) += pil-mba.o
obj-$(CONFIG_MSM_PIL_RIVA) += pil-riva.o
obj-$(CONFIG_MSM_PIL_TZAPPS) += pil-tzapps.o
obj-$(CONFIG_MSM_PIL_VIDC) += pil-vidc.o
obj-$(CONFIG_MSM_PIL_MODEM) += pil-modem.o
obj-$(CONFIG_MSM_PIL_DSPS) += pil-dsps.o
obj-$(CONFIG_MSM_PIL_GSS) += pil-gss.o
obj-$(CONFIG_MSM_PIL_PRONTO) += pil-pronto.o
obj-$(CONFIG_MSM_PIL_VENUS) += pil-venus.o
obj-$(CONFIG_ARCH_QSD8X50) += sirc.o
obj-$(CONFIG_ARCH_FSM9XXX) += sirc-fsm9xxx.o
obj-$(CONFIG_MSM_FIQ_SUPPORT) += fiq_glue.o
obj-$(CONFIG_MACH_TROUT) += board-trout-rfkill.o
obj-$(CONFIG_MSM_SDIO_AL) += sdio_al.o
obj-$(CONFIG_MSM_SDIO_AL) += sdio_al_test.o
obj-$(CONFIG_MSM_SDIO_AL) += sdio_al_dloader.o
obj-$(CONFIG_MSM_SDIO_DMUX) += sdio_dmux.o
obj-$(CONFIG_MSM_BAM_DMUX) += bam_dmux.o
obj-$(CONFIG_MSM_SMD_LOGGING) += smem_log.o
obj-$(CONFIG_MSM_IPC_LOGGING) += ipc_logging.o
ifdef CONFIG_DEBUG_FS
obj-$(CONFIG_MSM_IPC_LOGGING) += ipc_logging_debug.o
endif
obj-$(CONFIG_MSM_SMD) += smd.o smd_debug.o remote_spinlock.o smd_private.o
obj-y += socinfo.o
obj-y += htc_sysinfo.o
ifndef CONFIG_ARCH_MSM9615
ifndef CONFIG_ARCH_APQ8064
ifndef CONFIG_ARCH_MSM8960
ifndef CONFIG_ARCH_MSM8X60
ifndef CONFIG_ARCH_MSM8974
	obj-$(CONFIG_MSM_SMD) += pmic.o
	obj-$(CONFIG_MSM_ONCRPCROUTER) += rpc_hsusb.o rpc_pmapp.o rpc_fsusb.o
endif
endif
endif
endif
endif
obj-$(CONFIG_MSM_SDIO_TTY) += sdio_tty.o
obj-$(CONFIG_MSM_SMD_TTY) += smd_tty.o
obj-$(CONFIG_MSM_SMD_QMI) += smd_qmi.o
obj-$(CONFIG_MSM_SMD_PKT) += smd_pkt.o
obj-$(CONFIG_MSM_SDIO_CMUX) += sdio_cmux.o
obj-$(CONFIG_MSM_DSPS) += msm_dsps.o
obj-$(CONFIG_MSM_SDIO_CTL) += sdio_ctl.o
obj-$(CONFIG_MSM_SMD_NMEA) += smd_nmea.o
obj-$(CONFIG_MSM_RESET_MODEM) += reset_modem.o
obj-$(CONFIG_MSM_IPC_ROUTER_SMD_XPRT) += ipc_router_smd_xprt.o
obj-$(CONFIG_MSM_IPC_ROUTER_HSIC_XPRT) += ipc_router_hsic_xprt.o
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
obj-$(CONFIG_MSM7KV2_AUDIO) += qdsp5v2/
obj-$(CONFIG_MSM_RPCSERVER_HANDSET) += rpc_server_handset.o
obj-$(CONFIG_MSM_QDSP6) += qdsp6/
obj-$(CONFIG_MSM8X60_AUDIO) += qdsp6v2/
obj-$(CONFIG_MSM_AUDIO_QDSP6) += qdsp6v2/
obj-$(CONFIG_MSM_AUDIO_QDSP6V2) += qdsp6v2/
obj-$(CONFIG_MSM_HW3D) += hw3d.o
obj-$(CONFIG_PM) += pm-boot.o
obj-$(CONFIG_MSM_PM8X60) += pm-8x60.o pm-data.o
obj-$(CONFIG_MSM_IDLE_STATS) += pm-stats.o
obj-$(CONFIG_MSM_PM2) += pm2.o
obj-$(CONFIG_MSM_NOPM) += no-pm.o

obj-$(CONFIG_MSM_PCIE) += pcie.o pcie_irq.o

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
obj-$(CONFIG_ARCH_MSM8X60) += devices-msm8x60.o clock-local.o clock-8x60.o acpuclock-8x60.o clock-pll.o
obj-$(CONFIG_ARCH_MSM8X60) += clock-rpm.o
obj-$(CONFIG_ARCH_MSM8X60) += saw-regulator.o
obj-$(CONFIG_ARCH_MSM8X60) += footswitch-8x60.o
obj-$(CONFIG_ARCH_MSM8960) += htc_acoustic_8960.o
obj-$(CONFIG_MSM8X60_AUDIO) += htc_acoustic_8x60.o

ifdef CONFIG_MSM_RPM_REGULATOR
obj-y += rpm-regulator.o
obj-$(CONFIG_ARCH_MSM8X60) += rpm-regulator-8660.o
obj-$(CONFIG_ARCH_MSM8960) += rpm-regulator-8960.o
obj-$(CONFIG_ARCH_MSM9615) += rpm-regulator-9615.o
obj-$(CONFIG_ARCH_MSM8930) += rpm-regulator-8930.o
obj-$(CONFIG_ARCH_APQ8064) += rpm-regulator-8960.o
endif

obj-$(CONFIG_MSM_RPM_REGULATOR_SMD) += rpm-regulator-smd.o

ifdef CONFIG_MSM_SUBSYSTEM_RESTART
	obj-y += subsystem_notif.o
ifdef CONFIG_MSM8960_ONLY
	obj-y += subsystem_restart-8960.o
else ifdef CONFIG_MSM8930_ONLY
	obj-y += subsystem_restart-8960.o
else
	obj-y += subsystem_restart.o
endif
	obj-y += ramdump.o
	obj-$(CONFIG_ARCH_MSM8X60) += modem-8660.o lpass-8660.o
endif
obj-$(CONFIG_MSM_SYSMON_COMM) += sysmon.o
obj-$(CONFIG_MSM_MODEM_8960) += modem-8960.o
obj-$(CONFIG_MSM_LPASS_8960) += lpass-8960.o
obj-$(CONFIG_MSM_WCNSS_SSR_8960) += wcnss-ssr-8960.o
obj-$(CONFIG_MSM_GSS_SSR_8064) += gss-8064.o

ifdef CONFIG_CPU_IDLE
	obj-$(CONFIG_ARCH_APQ8064) += cpuidle.o
	obj-$(CONFIG_ARCH_MSM8960) += cpuidle.o
	obj-$(CONFIG_ARCH_MSM8X60) += cpuidle.o
	obj-$(CONFIG_ARCH_MSM9615) += cpuidle.o
	obj-$(CONFIG_ARCH_MSM8974) += cpuidle.o
endif

ifdef CONFIG_BT
	obj-$(CONFIG_MACH_MONARUDO) += board-monarudo-rfkill.o  htc_bdaddress.o
	obj-$(CONFIG_MACH_DELUXE_J) += board-deluxe_j-rfkill.o  htc_bdaddress.o
	obj-$(CONFIG_MACH_DELUXE_UB1) += board-deluxe_ub1-rfkill.o  htc_bdaddress.o
	obj-$(CONFIG_MACH_DELUXE_U) += board-deluxe_u-rfkill.o  htc_bdaddress.o
	obj-$(CONFIG_MACH_IMPRESSION_J) += board-impression_j-rfkill.o  htc_bdaddress.o
	obj-$(CONFIG_MACH_M7_UL) += board-m7-rfkill.o  htc_bdaddress.o htc_4335_wl_reg.o
	obj-$(CONFIG_MACH_M7_WLS) += board-m7wl-rfkill.o  htc_bdaddress.o htc_4335_wl_reg.o
	obj-$(CONFIG_MACH_M7_WLV) += board-m7wl-rfkill.o  htc_bdaddress.o htc_4335_wl_reg.o
	obj-$(CONFIG_MACH_M7_WLJ) += board-m7wl-rfkill.o  htc_bdaddress.o htc_4335_wl_reg.o
	obj-$(CONFIG_MACH_M7_DCG) += htc_bdaddress.o
	obj-$(CONFIG_MACH_M7_DUG) += htc_bdaddress.o
	obj-$(CONFIG_MACH_M7_DTU) += htc_bdaddress.o
	obj-$(CONFIG_MACH_M7C_DTU) += htc_bdaddress.o
	obj-$(CONFIG_MACH_M7C_DUG) += htc_bdaddress.o
	obj-$(CONFIG_MACH_M7C_DWG) += htc_bdaddress.o
	obj-$(CONFIG_MACH_DLP_DTU) += htc_bdaddress.o
	obj-$(CONFIG_MACH_DLP_DUG) += htc_bdaddress.o
	obj-$(CONFIG_MACH_DLP_DWG) += htc_bdaddress.o
	obj-$(CONFIG_MACH_T6_TL) += htc_bdaddress.o
	obj-$(CONFIG_MACH_T6_DUG) += htc_bdaddress.o
	obj-$(CONFIG_MACH_T6_DWG) += htc_bdaddress.o
	obj-$(CONFIG_MACH_DLXP_WL) += board-dlxp_wl-rfkill.o htc_bdaddress.o htc_4335_wl_reg.o
	obj-$(CONFIG_MACH_DLXP_UL) += board-dlxp_ul-rfkill.o htc_bdaddress.o htc_4335_wl_reg.o
endif

#ifdef CONFIG_MSM_CAMERA_V4L2
#	obj-$(CONFIG_ARCH_MSM8X60) += board-msm8x60-camera.o
#endif
obj-$(CONFIG_ARCH_FSM9XXX) += devices-fsm9xxx.o
obj-$(CONFIG_ARCH_FSM9XXX) += clock-fsm9xxx.o clock-local.o acpuclock-fsm9xxx.o
obj-$(CONFIG_ARCH_FSM9XXX) += dfe-fsm9xxx.o rfic-fsm9xxx.o
obj-$(CONFIG_ARCH_FSM9XXX) += restart-fsm9xxx.o xo-fsm9xxx.o

obj-$(CONFIG_MSM_WATCHDOG) += msm_watchdog.o
obj-$(CONFIG_MSM_WATCHDOG) += htc_watchdog_monitor.o
obj-$(CONFIG_MSM_WATCHDOG) += msm_watchdog_asm.o
obj-$(CONFIG_MSM_WATCHDOG_V2) += msm_watchdog_v2.o
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
board-7627a-all-objs += board-msm7627a-storage.o board-msm7627a-bt.o board-msm7627a-camera.o
board-7627a-all-objs += board-msm7627a-display.o board-msm7627a-wlan.o board-msm7627a-io.o
obj-$(CONFIG_MACH_MSM7X27A_RUMI3) += board-msm7x27a.o board-7627a-all.o
obj-$(CONFIG_MACH_MSM7X27A_SURF) += board-msm7x27a.o board-7627a-all.o
obj-$(CONFIG_MACH_MSM7X27A_FFA) += board-msm7x27a.o board-7627a-all.o
obj-$(CONFIG_MACH_MSM7627A_QRD1) += board-qrd7627a.o board-7627a-all.o
obj-$(CONFIG_MACH_MSM7627A_QRD3) += board-qrd7627a.o board-7627a-all.o
obj-$(CONFIG_MACH_MSM7627A_EVB) += board-qrd7627a.o board-7627a-all.o
obj-$(CONFIG_ARCH_MSM8625) += msm_smem_iface.o devices-msm7x27a.o clock-pcom-lookup.o mpm-8625.o
obj-$(CONFIG_MACH_MSM8625_RUMI3) += board-msm7x27a.o
obj-$(CONFIG_MACH_MSM8625_SURF) +=  board-msm7x27a.o board-7627a-all.o
obj-$(CONFIG_MACH_MSM8625_EVB) +=  board-qrd7627a.o board-7627a-all.o
obj-$(CONFIG_MACH_MSM8625_QRD7) +=  board-qrd7627a.o board-7627a-all.o
obj-$(CONFIG_MACH_MSM8625_FFA) += board-msm7x27a.o board-7627a-all.o
obj-$(CONFIG_MACH_MSM8625_EVT) += board-msm7x27a.o board-7627a-all.o
obj-$(CONFIG_ARCH_MSM7X30) += board-msm7x30.o devices-msm7x30.o memory_topology.o
obj-$(CONFIG_ARCH_MSM7X30) += clock-local.o clock-7x30.o acpuclock-7x30.o clock-pll.o
obj-$(CONFIG_MACH_MSM7X25_SURF) += board-msm7x27.o devices-msm7x25.o
obj-$(CONFIG_MACH_MSM7X25_FFA) += board-msm7x27.o devices-msm7x25.o
obj-$(CONFIG_ARCH_MSM8960) += clock-local.o clock-dss-8960.o clock-8960.o clock-rpm.o clock-pll.o
obj-$(CONFIG_ARCH_MSM8960) += footswitch-8x60.o
ifdef CONFIG_ARCH_APQ8064
obj-$(CONFIG_ARCH_APQ8064) += acpuclock-8064.o acpuclock-krait.o acpuclock-krait-debug.o
obj-$(CONFIG_MSM_CPU_PWRCTL) +=  msm_cpu_pwrctl.o
else
obj-$(CONFIG_ARCH_MSM8960) += acpuclock-8960.o
endif

obj-$(CONFIG_ARCH_RANDOM) += early_random.o

obj-$(CONFIG_ARCH_MSM8960) += memory_topology.o
obj-$(CONFIG_ARCH_MSM8960) += saw-regulator.o
obj-$(CONFIG_ARCH_MSM8960) += devices-8960.o htc_wifi_nvs.o devices_htc.o htc_awb_cal.o
obj-$(CONFIG_ARCH_MSM8930) += devices-8960.o htc_wifi_nvs.o devices_htc.o htc_awb_cal.o
obj-$(CONFIG_ARCH_APQ8064) += devices-8960.o devices-8064.o devices_htc.o htc_awb_cal.o
board-8960-all-objs += board-8960.o board-8960-camera.o board-8960-display.o board-8960-pmic.o board-8960-storage.o board-8960-gpiomux.o
board-8930-all-objs += board-8930.o board-8930-camera.o board-8930-display.o board-8930-pmic.o board-8930-storage.o board-8930-gpiomux.o devices-8930.o board-8930-gpu.o
board-8064-all-objs += board-8064.o board-8064-pmic.o board-8064-storage.o board-8064-gpiomux.o board-8064-camera.o board-8064-display.o board-8064-gpu.o
board-elite-all-objs += board-elite.o board-elite-gpiomux.o devices-8960.o board-8960-storage.o
board-evita_utl-all-objs += board-evita_utl.o board-evita_utl-gpiomux.o devices-8960.o board-8960-storage.o
board-k2_cl-all-objs +=  board-k2_cl-pmic.o board-k2_cl-storage.o board-k2_cl-gpiomux.o devices-8930.o
board-k2_plc_cl-all-objs +=  board-k2_plc_cl-pmic.o board-k2_plc_cl-storage.o board-k2_plc_cl-gpiomux.o devices-8930.o
board-tc2-all-objs +=  board-tc2-pmic.o board-tc2-storage.o board-tc2-gpiomux.o devices-8930.o
board-zara-all-objs +=  board-zara-pmic.o board-zara-storage.o board-zara-gpiomux.o devices-8930.o
board-m4-all-objs +=  board-m4-pmic.o board-m4-storage.o board-m4-gpiomux.o devices-8930.o
board-zip_cl-all-objs +=  board-zip_cl-pmic.o board-zip_cl-storage.o board-zip_cl-gpiomux.o devices-8930.o
board-k2_u-all-objs +=  board-k2_u-pmic.o board-k2_u-storage.o board-k2_u-gpiomux.o devices-8930.o
board-k2_ul-all-objs +=  board-k2_ul-pmic.o board-k2_ul-storage.o board-k2_ul-gpiomux.o devices-8930.o
board-operaul-all-objs +=  board-operaul-pmic.o board-operaul-storage.o board-operaul-gpiomux.o devices-8930.o
# board-monarudo-all-objs += board-monarudo.o board-monarudo-pmic.o board-monarudo-storage.o board-monarudo-gpiomux.o board-monarudo-display.o board-monarudo-gpu.o board-monarudo-keypad.o board-monarudo-wifi.o
board-monarudo-all-objs += board-monarudo.o board-monarudo-pmic.o board-monarudo-storage.o board-monarudo-gpiomux.o board-monarudo-display.o board-monarudo-gpu.o board-monarudo-keypad.o board-monarudo-wifi.o  board-monarudo-audio.o
board-deluxe_j-all-objs += board-deluxe_j.o board-deluxe_j-pmic.o board-deluxe_j-storage.o board-deluxe_j-gpiomux.o board-deluxe_j-keypad.o board-deluxe_j-wifi.o board-deluxe_j-audio.o
board-deluxe_ub1-all-objs += board-deluxe_ub1.o board-deluxe_ub1-pmic.o board-deluxe_ub1-storage.o board-deluxe_ub1-gpiomux.o board-deluxe_ub1-keypad.o board-deluxe_ub1-wifi.o board-deluxe_ub1-audio.o
board-deluxe_u-all-objs += board-deluxe_u.o board-deluxe_u-pmic.o board-deluxe_u-storage.o board-deluxe_u-gpiomux.o board-deluxe_u-display.o board-deluxe_u-gpu.o board-deluxe_u-keypad.o board-deluxe_u-wifi.o board-deluxe_u-audio.o
board-impression_j-all-objs += board-impression_j.o board-impression_j-pmic.o board-impression_j-storage.o board-impression_j-gpiomux.o board-impression_j-keypad.o board-impression_j-wifi.o board-impression_j-audio.o
board-m7-all-objs += board-m7.o board-m7-pmic.o board-m7-storage.o board-m7-gpiomux.o board-m7-keypad.o board-m7-wifi.o board-m7-audio.o
board-m7wls-all-objs += board-m7wls.o board-m7wls-pmic.o board-m7wl-storage.o board-m7wl-gpiomux.o board-m7wl-keypad.o board-m7wl-wifi.o board-m7wl-audio.o
board-m7wlv-all-objs += board-m7wlv.o board-m7wlv-pmic.o board-m7wl-storage.o board-m7wl-gpiomux.o board-m7wl-keypad.o board-m7wl-wifi.o board-m7wl-audio.o
board-m7wlj-all-objs += board-m7wlj.o board-m7wlj-pmic.o board-m7wlj-storage.o board-m7wlj-gpiomux.o board-m7wlj-keypad.o board-m7wl-wifi.o board-m7wlj-audio.o
board-m7dcg-all-objs += board-m7dcg.o board-m7dcg-pmic.o board-m7china-storage.o board-m7china-gpiomux.o board-m7dcg-keypad.o board-m7china-wifi.o board-m7china-audio.o board-m7evm-pmic.o board-m7evm.o
board-m7dug-all-objs += board-m7dug.o board-m7dug-pmic.o board-m7china-storage.o board-m7china-gpiomux.o board-m7dug-keypad.o board-m7china-wifi.o board-m7china-audio.o board-m7evm-pmic.o board-m7evm.o
board-m7dtu-all-objs += board-m7dtu.o board-m7dtu-pmic.o board-m7china-storage.o board-m7china-gpiomux.o board-m7dtu-keypad.o board-m7china-wifi.o board-m7china-audio.o board-m7evm-pmic.o board-m7evm.o
board-m7cdtu-all-objs += board-m7cdtu.o board-m7cdtu-pmic.o board-m7china-storage.o board-m7china-gpiomux.o board-m7cdtu-keypad.o board-m7china-wifi.o board-m7china-audio.o board-m7evm-pmic.o board-m7evm.o
board-m7cdug-all-objs += board-m7cdug.o board-m7cdug-pmic.o board-m7china-storage.o board-m7china-gpiomux.o board-m7cdug-keypad.o board-m7china-wifi.o board-m7china-audio.o board-m7evm-pmic.o board-m7evm.o
board-m7cdwg-all-objs += board-m7cdwg.o board-m7cdwg-pmic.o board-m7china-storage.o board-m7china-gpiomux.o board-m7cdwg-keypad.o board-m7china-wifi.o board-m7china-audio.o board-m7evm-pmic.o board-m7evm.o
board-t6tl-all-objs += board-t6tl.o board-t6tl-pmic.o board-t6china-storage.o board-t6china-gpiomux.o board-t6tl-keypad.o board-t6china-wifi.o board-t6china-audio.o
board-t6dug-all-objs += board-t6dug.o board-t6dug-pmic.o board-t6china-storage.o board-t6china-gpiomux.o board-t6dug-keypad.o board-t6china-wifi.o board-t6china-audio.o
board-t6dwg-all-objs += board-t6dwg.o board-t6dwg-pmic.o board-t6china-storage.o board-t6china-gpiomux.o board-t6dwg-keypad.o board-t6china-wifi.o board-t6china-audio.o
board-dlxp_wl-all-objs += board-dlxp_wl.o board-dlxp_wl-pmic.o board-dlxp_wl-storage.o board-dlxp_wl-gpiomux.o board-dlxp_wl-display.o board-dlxp_wl-gpu.o board-dlxp_wl-keypad.o board-dlxp_wl-wifi.o  board-dlxp_wl-audio.o
board-dlxp_ul-all-objs += board-dlxp_ul.o board-dlxp_ul-pmic.o board-dlxp_ul-storage.o board-dlxp_ul-gpiomux.o board-dlxp_ul-display.o board-dlxp_ul-gpu.o board-dlxp_ul-keypad.o board-dlxp_ul-wifi.o  board-dlxp_ul-audio.o
board-dlp_dtu-all-objs += board-dlp_dtu.o board-dlp_dtu-pmic.o board-dlp_china-storage.o board-dlp_china-gpiomux.o board-dlp_dtu-keypad.o board-m7china-wifi.o board-dlp_china-audio.o
board-dlp_dug-all-objs += board-dlp_dug.o board-dlp_dug-pmic.o board-dlp_china-storage.o board-dlp_china-gpiomux.o board-dlp_dug-keypad.o board-m7china-wifi.o board-dlp_china-audio.o
board-dlp_dwg-all-objs += board-dlp_dwg.o board-dlp_dwg-pmic.o board-dlp_china-storage.o board-dlp_china-gpiomux.o board-dlp_dwg-keypad.o board-m7china-wifi.o board-dlp_china-audio.o

ifdef CONFIG_FB_MSM
board-deluxe_j-all-objs += board-deluxe_j-display.o board-deluxe_j-gpu.o
board-deluxe_ub1-all-objs += board-deluxe_ub1-display.o board-deluxe_ub1-gpu.o
board-impression_j-all-objs += board-impression_j-display.o board-impression_j-gpu.o
board-m7-all-objs += board-m7-display.o board-m7-gpu.o
board-m7wls-all-objs += board-m7wl-display.o board-m7wl-gpu.o
board-m7wlv-all-objs += board-m7wl-display.o board-m7wl-gpu.o
board-m7wlj-all-objs += board-m7wl-display.o board-m7wl-gpu.o
board-m7dcg-all-objs += board-m7china-display.o board-m7china-gpu.o
board-m7dug-all-objs += board-m7china-display.o board-m7china-gpu.o
board-m7dtu-all-objs += board-m7china-display.o board-m7china-gpu.o
board-m7cdtu-all-objs += board-m7china-display.o board-m7china-gpu.o
board-m7cdug-all-objs += board-m7china-display.o board-m7china-gpu.o
board-m7cdwg-all-objs += board-m7china-display.o board-m7china-gpu.o
board-t6tl-all-objs += board-t6china-display.o board-t6china-gpu.o
board-t6dug-all-objs += board-t6china-display.o board-t6china-gpu.o
board-t6dwg-all-objs += board-t6china-display.o board-t6china-gpu.o
board-dlxp_wl-all-objs += board-dlxp_wl-display.o board-dlxp_wl-gpu.o
board-dlxp_ul-all-objs += board-dlxp_ul-display.o board-dlxp_ul-gpu.o
board-dlp_dtu-all-objs += board-dlp_china-display.o board-dlp_china-gpu.o
board-dlp_dug-all-objs += board-dlp_china-display.o board-dlp_china-gpu.o
board-dlp_dwg-all-objs += board-dlp_china-display.o board-dlp_china-gpu.o
board-operaul-all-objs += board-operaul-display.o board-operaul-gpu.o
board-tc2-all-objs += board-tc2-display.o board-tc2-gpu.o
board-zara-all-objs += board-zara-display.o board-zara-gpu.o
board-m4-all-objs += board-m4-display.o board-m4-gpu.o
board-zip_cl-all-objs += board-zip_cl-display.o board-zip_cl-gpu.o
board-k2_cl-all-objs += board-k2-display.o board-k2-gpu.o
board-k2_plc_cl-all-objs += board-k2-display.o board-k2-gpu.o
board-k2_u-all-objs += board-k2-display.o board-k2-gpu.o
board-k2_ul-all-objs += board-k2-display.o board-k2-gpu.o
endif

ifdef CONFIG_MSM_CAMERA
board-monarudo-all-objs += board-monarudo-camera.o
board-deluxe_j-all-objs += board-deluxe_j-camera.o
board-deluxe_ub1-all-objs += board-deluxe_ub1-camera.o
board-deluxe_u-all-objs += board-deluxe_u-camera.o
board-impression_j-all-objs += board-impression_j-camera.o
board-m7-all-objs += board-m7-camera.o
board-m7wls-all-objs += board-m7wl-camera.o
board-m7wlv-all-objs += board-m7wlv-camera.o
board-m7wlj-all-objs += board-m7wlj-camera.o
board-m7dcg-all-objs += board-m7china-camera.o
board-m7dug-all-objs += board-m7china-camera.o
board-m7dtu-all-objs += board-m7china-camera.o
board-m7cdtu-all-objs += board-m7china-camera.o
board-m7cdug-all-objs += board-m7china-camera.o
board-m7cdwg-all-objs += board-m7china-camera.o
board-dlxp_wl-all-objs += board-dlxp_wl-camera.o
board-dlxp_ul-all-objs += board-dlxp_ul-camera.o
board-dlp_dtu-all-objs += board-dlp_china-camera.o
board-dlp_dug-all-objs += board-dlp_china-camera.o
board-dlp_dwg-all-objs += board-dlp_china-camera.o
board-t6tl-all-objs += board-t6china-camera.o
board-t6dug-all-objs += board-t6china-camera.o
board-t6dwg-all-objs += board-t6china-camera.o
obj-$(CONFIG_MACH_ZARA) += board-zara-camera.o
endif
obj-$(CONFIG_MACH_MSM8960_SIM) += board-8960-all.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8960_RUMI3) += board-8960-all.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8960_CDP) += board-8960-all.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8960_MTP) += board-8960-all.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8960_FLUID) += board-8960-all.o board-8960-regulator.o
obj-$(CONFIG_MACH_MSM8930_CDP) += board-8930-all.o board-8930-regulator.o
obj-$(CONFIG_MACH_MSM8930_MTP) += board-8930-all.o board-8930-regulator.o
obj-$(CONFIG_MACH_MSM8930_FLUID) += board-8930-all.o board-8930-regulator.o
obj-$(CONFIG_PM8921_BMS) += bms-batterydata.o bms-batterydata-desay.o
obj-$(CONFIG_MACH_APQ8064_SIM) += board-8064-all.o board-8064-regulator.o
obj-$(CONFIG_MACH_APQ8064_RUMI3) += board-8064-all.o board-8064-regulator.o
obj-$(CONFIG_MACH_APQ8064_CDP) += board-8064-all.o board-8064-regulator.o
obj-$(CONFIG_MACH_APQ8064_MTP) += board-8064-all.o board-8064-regulator.o
obj-$(CONFIG_MACH_APQ8064_LIQUID) += board-8064-all.o board-8064-regulator.o
obj-$(CONFIG_MACH_MPQ8064_HRD) += board-8064-all.o board-8064-regulator.o
obj-$(CONFIG_MACH_MPQ8064_DTV) += board-8064-all.o board-8064-regulator.o
obj-$(CONFIG_MACH_MONARUDO) += board-monarudo-all.o board-monarudo-regulator.o
obj-$(CONFIG_MACH_DELUXE_J) += board-deluxe_j-all.o board-deluxe_j-regulator.o
obj-$(CONFIG_MACH_DELUXE_UB1) += board-deluxe_ub1-all.o board-deluxe_ub1-regulator.o
obj-$(CONFIG_MACH_DELUXE_U) += board-deluxe_u-all.o board-deluxe_u-regulator.o
obj-$(CONFIG_MACH_IMPRESSION_J) += board-impression_j-all.o board-impression_j-regulator.o
obj-$(CONFIG_MACH_M7_UL) += board-m7-all.o board-m7-regulator.o
obj-$(CONFIG_MACH_M7_WLS) += board-m7wls-all.o board-m7wl-regulator.o
obj-$(CONFIG_MACH_M7_WLV) += board-m7wlv-all.o board-m7wl-regulator.o
obj-$(CONFIG_MACH_M7_WLJ) += board-m7wlj-all.o board-m7wlj-regulator.o
obj-$(CONFIG_MACH_M7_DCG) += board-m7dcg-all.o board-m7china-regulator.o
obj-$(CONFIG_MACH_M7_DUG) += board-m7dug-all.o board-m7china-regulator.o
obj-$(CONFIG_MACH_M7_DTU) += board-m7dtu-all.o board-m7china-regulator.o
obj-$(CONFIG_MACH_M7C_DTU) += board-m7cdtu-all.o board-m7china-regulator.o
obj-$(CONFIG_MACH_M7C_DUG) += board-m7cdug-all.o board-m7china-regulator.o
obj-$(CONFIG_MACH_M7C_DWG) += board-m7cdwg-all.o board-m7china-regulator.o
obj-$(CONFIG_MACH_T6_TL) += board-t6tl-all.o board-t6china-regulator.o
obj-$(CONFIG_MACH_T6_DUG) += board-t6dug-all.o board-t6china-regulator.o
obj-$(CONFIG_MACH_T6_DWG) += board-t6dwg-all.o board-t6china-regulator.o
obj-$(CONFIG_MACH_DLXP_WL) += board-dlxp_wl-all.o board-dlxp_wl-regulator.o
obj-$(CONFIG_MACH_DLXP_UL) += board-dlxp_ul-all.o board-dlxp_ul-regulator.o
obj-$(CONFIG_MACH_DLP_DTU) += board-dlp_dtu-all.o board-dlp_china-regulator.o
obj-$(CONFIG_MACH_DLP_DUG) += board-dlp_dug-all.o board-dlp_china-regulator.o
obj-$(CONFIG_MACH_DLP_DWG) += board-dlp_dwg-all.o board-dlp_china-regulator.o

obj-$(CONFIG_MACH_MONARUDO) += htc_util.o
obj-$(CONFIG_MACH_DELUXE_J) += htc_util.o
obj-$(CONFIG_MACH_DELUXE_UB1) += htc_util.o
obj-$(CONFIG_MACH_DELUXE_U) += htc_util.o
obj-$(CONFIG_MACH_IMPRESSION_J) += htc_util.o
obj-$(CONFIG_MACH_M7_UL) += htc_util.o
obj-$(CONFIG_MACH_M7_WLS) += htc_util.o
obj-$(CONFIG_MACH_M7_WLV) += htc_util.o
obj-$(CONFIG_MACH_M7_WLJ) += htc_util.o
obj-$(CONFIG_MACH_M7_DCG) += htc_util.o
obj-$(CONFIG_MACH_M7_DUG) += htc_util.o
obj-$(CONFIG_MACH_M7_DTU) += htc_util.o
obj-$(CONFIG_MACH_M7C_DTU) += htc_util.o
obj-$(CONFIG_MACH_M7C_DUG) += htc_util.o
obj-$(CONFIG_MACH_M7C_DWG) += htc_util.o
obj-$(CONFIG_MACH_T6_TL) += htc_util.o
obj-$(CONFIG_MACH_T6_DUG) += htc_util.o
obj-$(CONFIG_MACH_T6_DWG) += htc_util.o
obj-$(CONFIG_MACH_DLXP_WL) += htc_util.o
obj-$(CONFIG_MACH_DLXP_UL) += htc_util.o
obj-$(CONFIG_MACH_DLP_DTU) += htc_util.o
obj-$(CONFIG_MACH_DLP_DUG) += htc_util.o
obj-$(CONFIG_MACH_DLP_DWG) += htc_util.o
obj-$(CONFIG_MACH_ELITE) += board-elite.o board-elite-regulator.o board-elite-gpiomux.o board-elite-storage.o board-elite-audio.o board-elite-camera.o
obj-$(CONFIG_MACH_ELITE) += board-elite-keypad.o
obj-$(CONFIG_MACH_ELITE) += htc_bdaddress.o
obj-$(CONFIG_MACH_ELITE) += htc_util.o
obj-$(CONFIG_MACH_EVITA_UTL) += board-evita_utl.o board-evita_utl-regulator.o board-evita_utl-gpiomux.o board-evita_utl-storage.o board-evita_utl-audio.o board-evita_utl-camera.o
obj-$(CONFIG_MACH_EVITA_UTL) += board-evita_utl-keypad.o
obj-$(CONFIG_MACH_EVITA_UTL) += htc_bdaddress.o
obj-$(CONFIG_MACH_EVITA_UTL) += htc_util.o
obj-$(CONFIG_MACH_VILLE) += board-ville.o board-ville-regulator.o board-ville-gpiomux.o board-ville-storage.o board-ville-audio.o board-ville-camera.o
obj-$(CONFIG_MACH_VILLE) += board-ville-keypad.o
obj-$(CONFIG_MACH_VILLE) += htc_bdaddress.o
obj-$(CONFIG_MACH_VILLE) += htc_util.o
obj-$(CONFIG_MACH_FIGHTER) += board-fighter.o board-fighter-regulator.o board-fighter-gpiomux.o board-fighter-storage.o board-fighter-audio.o board-fighter-camera.o
obj-$(CONFIG_MACH_FIGHTER) += board-fighter-keypad.o
obj-$(CONFIG_MACH_FIGHTER) += htc_bdaddress.o
obj-$(CONFIG_MACH_FIGHTER) += htc_util.o
obj-$(CONFIG_MACH_JET) += board-jet.o board-jet-regulator.o board-jet-gpiomux.o board-jet-storage.o board-jet-audio.o board-jet-camera.o
obj-$(CONFIG_MACH_JET) += board-jet-keypad.o
obj-$(CONFIG_MACH_JET) += htc_bdaddress.o
obj-$(CONFIG_MACH_JET) += htc_util.o
obj-$(CONFIG_MACH_JEWEL_DD) += board-jel_dd.o board-jel_dd-regulator.o board-jel_dd-gpiomux.o board-jel_dd-storage.o board-jel_dd-audio.o board-jel_dd-camera.o
obj-$(CONFIG_MACH_JEWEL_DD) += board-jel_dd-keypad.o board-jel_dd-gsm-radio.o
obj-$(CONFIG_MACH_JEWEL_DD) += htc_bdaddress.o
obj-$(CONFIG_MACH_JEWEL_DD) += htc_util.o
obj-$(CONFIG_MACH_VALENTE_WX) += board-valente_wx.o board-valente_wx-regulator.o board-valente_wx-gpiomux.o board-valente_wx-storage.o board-valente_wx-audio.o board-valente_wx-camera.o
obj-$(CONFIG_MACH_VALENTE_WX) += board-valente_wx-keypad.o
obj-$(CONFIG_MACH_VALENTE_WX) += htc_bdaddress.o
obj-$(CONFIG_MACH_VALENTE_WX) += htc_util.o
obj-$(CONFIG_MACH_VALENTE_WX_C9) += board-valente_wx_c9.o board-valente_wx_c9-regulator.o board-valente_wx_c9-gpiomux.o board-valente_wx_c9-storage.o board-valente_wx_c9-audio.o board-valente_wx_c9-camera.o
obj-$(CONFIG_MACH_VALENTE_WX_C9) += board-valente_wx_c9-keypad.o
obj-$(CONFIG_MACH_VALENTE_WX_C9) += htc_bdaddress.o
obj-$(CONFIG_MACH_VALENTE_WX_C9) += htc_util.o

obj-$(CONFIG_MACH_K2_U) += board-k2_u.o board-k2_u-all.o board-k2_u-regulator.o board-k2_ul-audio.o
obj-$(CONFIG_MACH_K2_U) += htc_bdaddress.o board-k2_u-rfkill.o htc_sleep_clk.o htc_util.o board-k2-camera.o
#obj-$(CONFIG_MACH_K2_U) += htc_acoustic.o
obj-$(CONFIG_MACH_K2_U) += board-k2_u-keypad.o
obj-$(CONFIG_MACH_K2_U) += board-k2_u-mmc.o board-k2_u-wifi.o htc_wifi_nvs.o

obj-$(CONFIG_MACH_TC2) += board-tc2.o board-tc2-all.o board-tc2-regulator.o board-tc2-audio.o
obj-$(CONFIG_MACH_TC2) += htc_bdaddress.o board-tc2-rfkill.o htc_sleep_clk.o htc_util.o board-tc2-camera.o
#obj-$(CONFIG_MACH_TC2) += htc_acoustic.o
obj-$(CONFIG_MACH_TC2) += board-tc2-keypad.o
obj-$(CONFIG_MACH_TC2) += board-tc2-mmc.o board-tc2-wifi.o htc_wifi_nvs.o

obj-$(CONFIG_MACH_ZARA) += board-zara.o board-zara-all.o board-zara-regulator.o board-zara-audio.o
obj-$(CONFIG_MACH_ZARA) += htc_bdaddress.o board-zara-rfkill.o htc_sleep_clk.o htc_util.o
obj-$(CONFIG_MACH_ZARA) += board-zara-keypad.o
obj-$(CONFIG_MACH_ZARA) += board-zara-mmc.o board-zara-wifi.o htc_wifi_nvs.o




obj-$(CONFIG_MACH_M4_UL) += board-m4.o board-m4-all.o board-m4-regulator.o board-m4-audio.o
obj-$(CONFIG_MACH_M4_UL) += htc_bdaddress.o board-m4-rfkill.o htc_sleep_clk.o htc_util.o board-m4-camera.o
#obj-$(CONFIG_MACH_M4_UL) += htc_acoustic.o
obj-$(CONFIG_MACH_M4_UL) += board-m4-keypad.o
obj-$(CONFIG_MACH_M4_UL) += board-m4-mmc.o board-m4-wifi.o htc_wifi_nvs.o

obj-$(CONFIG_MACH_ZIP_CL) += board-zip_cl.o board-zip_cl-all.o board-zip_cl-regulator.o board-zip_cl-audio.o
obj-$(CONFIG_MACH_ZIP_CL) += htc_bdaddress.o board-zip_cl-rfkill.o htc_sleep_clk.o htc_util.o board-zip_cl-camera.o
#obj-$(CONFIG_MACH_ZIP_CL) += htc_acoustic.o
obj-$(CONFIG_MACH_ZIP_CL) += board-zip_cl-keypad.o
obj-$(CONFIG_MACH_ZIP_CL) += board-zip_cl-mmc.o board-zip_cl-wifi.o htc_wifi_nvs.o

obj-$(CONFIG_MACH_K2_CL) += board-k2_cl.o board-k2_cl-all.o board-k2_cl-regulator.o board-k2_cl-audio.o
obj-$(CONFIG_MACH_K2_CL) += htc_bdaddress.o board-k2_cl-rfkill.o htc_sleep_clk.o htc_util.o board-k2-camera.o
#obj-$(CONFIG_MACH_K2_CL) += htc_acoustic.o
obj-$(CONFIG_MACH_K2_CL) += board-k2_cl-keypad.o
obj-$(CONFIG_MACH_K2_CL) += board-k2_cl-mmc.o board-k2_cl-wifi.o htc_wifi_nvs.o

obj-$(CONFIG_MACH_K2_PLC_CL) += board-k2_plc_cl.o board-k2_plc_cl-all.o board-k2_plc_cl-regulator.o board-k2_plc_cl-audio.o
obj-$(CONFIG_MACH_K2_PLC_CL) += htc_bdaddress.o board-k2_plc_cl-rfkill.o htc_sleep_clk.o htc_util.o board-k2-camera.o
#obj-$(CONFIG_MACH_K2_PLC_CL) += htc_acoustic.o
obj-$(CONFIG_MACH_K2_PLC_CL) += board-k2_plc_cl-keypad.o
obj-$(CONFIG_MACH_K2_PLC_CL) += board-k2_plc_cl-mmc.o board-k2_plc_cl-wifi.o htc_wifi_nvs.o

obj-$(CONFIG_MACH_K2_UL) += board-k2_ul.o board-k2_ul-all.o board-k2_ul-regulator.o board-k2_ul-audio.o
obj-$(CONFIG_MACH_K2_UL) += htc_bdaddress.o board-k2_ul-rfkill.o htc_sleep_clk.o htc_util.o board-k2-camera.o
#obj-$(CONFIG_MACH_K2_UL) += htc_acoustic.o
obj-$(CONFIG_MACH_K2_UL) += board-k2_ul-keypad.o
obj-$(CONFIG_MACH_K2_UL) += board-k2_ul-mmc.o board-k2_ul-wifi.o htc_wifi_nvs.o

obj-$(CONFIG_MACH_OPERAUL) += board-operaul.o board-operaul-all.o board-operaul-regulator.o board-operaul-audio.o
obj-$(CONFIG_MACH_OPERAUL) += htc_bdaddress.o board-operaul-rfkill.o htc_sleep_clk.o htc_util.o board-operaul-camera.o
#obj-$(CONFIG_MACH_OPERAUL) += htc_acoustic.o
obj-$(CONFIG_MACH_OPERAUL) += board-operaul-keypad.o
obj-$(CONFIG_MACH_OPERAUL) += board-operaul-mmc.o board-operaul-wifi.o htc_wifi_nvs.o
obj-$(CONFIG_ARCH_MSM9615) += board-9615.o devices-9615.o board-9615-regulator.o board-9615-gpiomux.o board-9615-storage.o board-9615-display.o
obj-$(CONFIG_ARCH_MSM9615) += clock-local.o clock-9615.o acpuclock-9615.o clock-rpm.o clock-pll.o
obj-$(CONFIG_ARCH_MSM8974) += board-8974.o board-dt.o board-8974-regulator.o board-8974-gpiomux.o
obj-$(CONFIG_ARCH_MSM8974) += acpuclock-krait.o acpuclock-8974.o
obj-$(CONFIG_ARCH_MSM8974) += clock-local2.o clock-pll.o clock-8974.o clock-rpm.o clock-voter.o
obj-$(CONFIG_ARCH_MSM8974) += gdsc.o
obj-$(CONFIG_ARCH_MSM9625) += board-9625.o board-9625-gpiomux.o

ifeq ($(CONFIG_WIMAX),y)
obj-$(CONFIG_MACH_VALENTE_WX) += board-valente_wx-mmc.o  board-common-wimax.o
obj-$(CONFIG_MACH_VALENTE_WX_C9) += board-valente_wx_c9-mmc.o  board-common-wimax.o
endif

obj-$(CONFIG_MACH_SAPPHIRE) += board-sapphire.o board-sapphire-gpio.o
obj-$(CONFIG_MACH_SAPPHIRE) += board-sapphire-keypad.o board-sapphire-panel.o
obj-$(CONFIG_MACH_SAPPHIRE) += board-sapphire-mmc.o board-sapphire-wifi.o
obj-$(CONFIG_MACH_SAPPHIRE) += board-sapphire-rfkill.o msm_vibrator.o

obj-$(CONFIG_MACH_VILLEC2) += board-villec2.o devices_htc.o
obj-$(CONFIG_MACH_VILLEC2) += board-villec2-keypad.o board-villec2-gpiomux.o
obj-$(CONFIG_MACH_VILLEC2) += board-villec2-camera.o htc_awb_cal.o
obj-$(CONFIG_MACH_VILLEC2) += htc_bdaddress.o board-villec2-rfkill.o
# Broadcom 
obj-$(CONFIG_MACH_VILLEC2) += board-villec2-mmc.o board-villec2-wifi.o htc_wifi_nvs.o 
#obj-$(CONFIG_MACH_VILLEC2) += board-villec2-mmc.o
obj-$(CONFIG_MACH_VILLEC2) += htc_util.o
obj-$(CONFIG_MACH_VILLEC2) += board-villec2-audio.o

CFLAGS_msm_vibrator.o += -Idrivers/staging/android
CFLAGS_board-9615.o += -Idrivers/usb/gadget

obj-$(CONFIG_ARCH_FSM9XXX) += board-fsm9xxx.o

obj-$(CONFIG_TROUT_BATTCHG) += htc_battery.o
obj-$(CONFIG_HTC_BATT_CORE) += htc_battery_core.o
obj-$(CONFIG_HTC_BATT8x60) += htc_battery_8x60.o
obj-$(CONFIG_HTC_BATT_8960) += htc_battery_8960.o htc_battery_cell.o

obj-$(CONFIG_HTC_PWRSINK) += htc_pwrsink.o
obj-$(CONFIG_HTC_HEADSET_MGR) += htc_headset_mgr.o
obj-$(CONFIG_HTC_HEADSET_PMIC) += htc_headset_pmic.o
obj-$(CONFIG_HTC_HEADSET_GPIO) += htc_headset_gpio.o
obj-$(CONFIG_HTC_HEADSET_8X60) += htc_headset_8x60.o
obj-$(CONFIG_HTC_HEADSET_ONE_WIRE) += htc_headset_one_wire.o
obj-$(CONFIG_HTC_HEADSET) += htc_headset.o
obj-$(CONFIG_MSM_RMT_STORAGE_CLIENT) += rmt_storage_client.o
obj-$(CONFIG_MSM_SDIO_SMEM) += sdio_smem.o
obj-$(CONFIG_MSM_RPM) += rpm.o
ifdef CONFIG_MSM_RPM
	obj-$(CONFIG_ARCH_APQ8064) += rpm_resources.o
	obj-$(CONFIG_ARCH_MSM8960) += rpm_resources.o
	obj-$(CONFIG_ARCH_MSM8X60) += rpm_resources.o
	obj-$(CONFIG_ARCH_MSM9615) += rpm_resources.o
endif
ifdef CONFIG_MSM_RPM_SMD
	obj-$(CONFIG_ARCH_MSM8974) += lpm_levels.o lpm_resources.o
endif
obj-$(CONFIG_MSM_MPM) += mpm.o
obj-$(CONFIG_MSM_RPM_STATS_LOG) += rpm_stats.o
obj-$(CONFIG_MSM_RPM_LOG) += rpm_log.o
obj-$(CONFIG_MSM_TZ_LOG) += tz_log.o
obj-$(CONFIG_MSM_8X60_TZ_LOG) += tz_log_8x60.o
obj-$(CONFIG_MSM_XO) += msm_xo.o
obj-$(CONFIG_MSM_BUS_SCALING) += msm_bus/
obj-$(CONFIG_MSM_BUSPM_DEV) += msm-buspm-dev.o

obj-$(CONFIG_MSM_IOMMU)		+= devices-iommu.o iommu_domains.o

ifdef CONFIG_VCM
obj-$(CONFIG_ARCH_MSM8X60) += board-msm8x60-vcm.o
endif
obj-$(CONFIG_MSM_OCMEM) += ocmem.o ocmem_allocator.o ocmem_notifier.o
obj-$(CONFIG_MSM_OCMEM) += ocmem_sched.o ocmem_api.o

obj-$(CONFIG_ARCH_MSM7X27) += gpiomux-7x27.o gpiomux-v1.o gpiomux.o
obj-$(CONFIG_ARCH_MSM7X30) += gpiomux-7x30.o gpiomux-v1.o gpiomux.o
obj-$(CONFIG_ARCH_QSD8X50) += gpiomux-8x50.o gpiomux-v1.o gpiomux.o
obj-$(CONFIG_ARCH_MSM8X60) += gpiomux-8x60.o gpiomux-v2.o gpiomux.o
obj-$(CONFIG_ARCH_MSM8960) += gpiomux-v2.o gpiomux.o
obj-$(CONFIG_ARCH_APQ8064) += gpiomux-v2.o gpiomux.o
obj-$(CONFIG_ARCH_MSM9615) += gpiomux-v2.o gpiomux.o
obj-$(CONFIG_ARCH_MSM8974) += gpiomux-v2.o gpiomux.o
obj-$(CONFIG_ARCH_MSM9625) += gpiomux-v2.o gpiomux.o


obj-$(CONFIG_MSM_SLEEP_STATS) += idle_stats.o
obj-$(CONFIG_MSM_SLEEP_STATS_DEVICE) += idle_stats_device.o
obj-$(CONFIG_MSM_DCVS) += msm_dcvs_scm.o msm_dcvs.o msm_dcvs_idle.o
obj-$(CONFIG_MSM_RUN_QUEUE_STATS) += msm_rq_stats.o
obj-$(CONFIG_MSM_SHOW_RESUME_IRQ) += msm_show_resume_irq.o
obj-$(CONFIG_BT_MSM_PINTEST)  += btpintest.o
obj-$(CONFIG_MSM_FAKE_BATTERY) += fish_battery.o
obj-$(CONFIG_MSM_RPC_VIBRATOR) += msm_vibrator.o
ifdef CONFIG_ARCH_APQ8064
	obj-$(CONFIG_MSM_NATIVE_RESTART) += restart-apq8064.o
else
	obj-$(CONFIG_MSM_NATIVE_RESTART) += restart.o
endif
obj-$(CONFIG_MSM_NATIVE_RESTART) += htc_restart_handler.o

obj-$(CONFIG_MSM_PROC_COMM_REGULATOR) += proccomm-regulator.o
ifdef CONFIG_MSM_PROC_COMM_REGULATOR
obj-$(CONFIG_MACH_MSM7X27_SURF) += board-msm7627-regulator.o
obj-$(CONFIG_MACH_MSM7X27_FFA) += board-msm7627-regulator.o
obj-$(CONFIG_ARCH_MSM7X30) += board-msm7x30-regulator.o
obj-$(CONFIG_ARCH_MSM7X27A) += board-msm7x27a-regulator.o
endif

obj-$(CONFIG_ARCH_APQ8064) += mdm2.o mdm_common.o
obj-$(CONFIG_QSC_MODEM) += qsc_ctrl.o
obj-$(CONFIG_MSM_RTB) += msm_rtb.o
obj-$(CONFIG_MSM_CACHE_ERP) += cache_erp.o
obj-$(CONFIG_MSM_EBI_ERP) += ebi_erp.o
obj-$(CONFIG_MSM_CACHE_DUMP) += msm_cache_dump.o

obj-$(CONFIG_MSM_HSIC_SYSMON) += hsic_sysmon.o
obj-$(CONFIG_MSM_HSIC_SYSMON_TEST) += hsic_sysmon_test.o

obj-$(CONFIG_MSM_RPM_SMD) += rpm-smd.o

obj-$(CONFIG_PERFLOCK) += perflock.o
obj-y += htc_ramdump.o

obj-$(CONFIG_HTC_MONITOR) += htc_monitor.o

obj-y += display/

obj-$(CONFIG_IO_FOOTPRINT) += io_footprint.o

obj-$(CONFIG_IOEXTENDER) += ADP5585_ioextender.o

#PWR2_ADD, @CPU Sniffer: CPU usage statistics.
obj-y += htc_cpu_usage_stats.o

obj-$(CONFIG_MSM_IPC_ROUTER_SECURITY)+= msm_ipc_router_security.o
