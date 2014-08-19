#ifndef I915_REG_H
#define I915_REG_H
#define EXCC 					0x02028
#define RING_BUFFER_TAIL 		0x02030
#define RING_BUFFER_HEAD 		0x02034
#define RING_BUFFER_START		0x02038
#define RCS_RING_BUFFER_CTL		0x0203
#define VCS_RING_BUFFER_CTL		0x1203
#define VECS_RING_BUFFER_CTL	0x1A03
#define VBCS_RING_BUFFER_CTL	0x2203
#define RVSYNC					0x02040
#define RBSYNC					0x02044
#define RVESYNC					0x02048//page 36
#define NOPID					0x02094
#define HWSTAM					0x02098
#define MI_MODE					0x0209C//Rendermode reg for sw interface
#define FF_MODE					0x020A0
#define IMR						0x020A8
#define EIR						0x020B0
#define EMR						0x0B4
const char *reg_name[0x140000>>2];
unsigned char omitted[0x140000];
unsigned int supressed[0x140000];
void init_reg_profile(void)
{
	memset(omitted, 0x0, 0x140000);
	memset(reg_name, NULL, sizeof(const char *) * (0x140000>>2));
	reg_name[0x02028>>2]="EXEC";
	reg_name[0x02030>>2]="RING_BUFFER_TAIL";
	reg_name[0x02034>>2]="RING_BUFFER_HEAD";
	reg_name[0x02038>>2]="RING_BUFFER_START";
	reg_name[0x0203>>2]="RCS_RING_BUFFER_CTL";
	reg_name[0x1203>>2]="VCS_RING_BUFFER_CTL";
	reg_name[0x1A03>>2]="VECS_RING_BUFFER_CTL";
	reg_name[0x2203>>2]="VBCS_RING_BUFFER_CTL";
	reg_name[0x02040>>2]="RVSYNC";
	reg_name[0x02044>>2]="RBSYNC";
	reg_name[0x02048>>2]="RVESYNC";
	reg_name[0x02074>>2]="ACTHD";
	reg_name[0x02094>>2]="NOPID";
	reg_name[0x02098>>2]="HWSTAM";
	reg_name[0x0209C>>2]="MI_MODE";
	reg_name[0x020A0>>2]="FF_MODE";
	reg_name[0x020A8>>2]="IMR";
	reg_name[0x020B0>>2]="EIR";
	reg_name[0x020B4>>2]="EMR";
	reg_name[0x020B8>>2]="ESR";
	reg_name[0x020C0>>2]="INSTPM";
	reg_name[0x020CC>>2]="WAIT_FOR_RC6_EXIT";
	reg_name[0x02110>>2]="RCS_BB_STATE";
	reg_name[0x02114>>2]="SBB_ADDR";
	reg_name[0x02118>>2]="SBB_STATE";
	reg_name[0x0212C>>2]="GAFS_MODE";
	reg_name[0x02134>>2]="RCS_UHPTR";
	reg_name[0x12134>>2]="VCS_UHPTR";
	reg_name[0x1A134>>2]="VECS_UHPTR";
	reg_name[0x22134>>2]="BCS_UHPTR";
	reg_name[0x0213C>>2]="SBB_PREEMPT_ADDR";
	reg_name[0x02140>>2]="RCS_BB_ADDR";
	reg_name[0x12140>>2]="VCS_BB_ADDR";
	reg_name[0x1A140>>2]="VECS_BB_ADDR";
	reg_name[0x22140>>2]="BCS_BB_ADDR";
	reg_name[0x02148>>2]="RCS_BB_PREEMPT_ADDR";
	reg_name[0x0214C>>2]="RCS_RING_BUFFER_HEAD_PREEMPT_REG";
	reg_name[0x02150>>2]="BB_START_ADDR";
	reg_name[0x02154>>2]="BB_ADDR_DIFF";
	reg_name[0x02158>>2]="BB_OFFSET";
	reg_name[0x0215C]="RS_PREEMPT_STATUS";
	reg_name[0x02170>>2]="BB_START_ADDR_UDW";
	reg_name[0x02178>>2]="PR_CTR_CTL";
	reg_name[0x0217C>>2]="PR_CTR_THRSH";
	reg_name[0x02180>>2]="CCID";//important 4KB aligned
	reg_name[0x02190>>2]="PR_CTR";
	reg_name[0x021A8>>2]="CXT_SIZE";	
		
	reg_name[0x021B4>>2]="RS_CXT_OFFSET";
	reg_name[0x021B8>>2]="URB_CXT_OFFSET";
	reg_name[0x02214>>2]="MI_PREDICATE_RESULT_2";
	reg_name[0x02220>>2]="PP_DCLV";
	reg_name[0x0222C>>2]="MTCH_CID_RST";
	reg_name[0x02290>>2]="TS_GPGPU_THREADS_DISPATCHED";
	reg_name[0x0229C>>2]="GFX_MODE";
	reg_name[0x022AC>>2]="CSPWRFSM";
	reg_name[0x022C8>>2]="PS_INVOCATION_COUNT_SLICE0";
	reg_name[0x022D8>>2]="PS_DEPTH_COUNT_SLICE0";
	reg_name[0x022F0>>2]="PS_INVOCATION_COUNT_SLICE1";
	reg_name[0x022F8>>2]="PS_DEPTH_COUNT_SLICE1";
	reg_name[0x02300>>2]="HS_INVOCATION_COUNT";
	reg_name[0x02308>>2]="HS_DEPTH_COUNT";		
	reg_name[0x02310>>2]="IA_VERTICES_COUNT";
	reg_name[0x02318>>2]="IA_PRIMITIVES_COUNT";
	reg_name[0x02320>>2]="VS_INVOCATION_COUNT";//no VS_PRIMITIVE
	reg_name[0x02328>>2]="GS_INVOCATION_COUNT";
	reg_name[0x02330>>2]="GS_PRIMITIVES_COUNT";
	reg_name[0x02338>>2]="CL_INVOCATION_COUNT";
	reg_name[0x02340>>2]="CL_PRIMITIVES_COUNT";
	reg_name[0x02348>>2]="PS_INVOCATION_COUNT";
	reg_name[0x02350>>2]="PS_DEPTH_COUNT";
	reg_name[0x02358>>2]="TIMESTAMP";
	reg_name[0x02360>>2]="OACONTROL";
	reg_name[0x02364>>2]="OASTATUS1";
	reg_name[0x02368>>2]="OASTATUS2";
	reg_name[0x023B0>>2]="OABUFFER";
	reg_name[0x02400>>2]="MI_PREDICATE_SRC0";
	reg_name[0x02408>>2]="MI_PREDICATE_SRC1";
	reg_name[0x02410>>2]="MI_PREDICATE_DATA";
	reg_name[0x02418>>2]="MI_PREDICATE_RESULT";
	reg_name[0x0241C>>2]="MI_PREDICATE_RESULT_1";
	reg_name[0x02420>>2]="3DPRIM_END_OFFSET";
	reg_name[0x02430>>2]="3DPRIM_START_VERTEX";
	reg_name[0x02434>>2]="3DPRIM_VERTEX_COUNT";
	reg_name[0x02438>>2]="3DPRIM_INSTANCE_COUNT";
	reg_name[0x0243C>>2]="3DPRIM_START_INSTANCE";
	reg_name[0x02440>>2]="3DPRIM_BASE_VERTEX";
	reg_name[0x02470>>2]="VFSKPD";
	reg_name[0x02480>>2]="BTP_PRODUCE_COUNT";
	reg_name[0x02484>>2]="DX9CONST_PRODUCE_COUNT";
	reg_name[0x0248C>>2]="GATHER_CONST_PRODUCE_COUNT";
	reg_name[0x02490>>2]="BTP_PARSE_COUNT";
	reg_name[0x02494>>2]="FX9CONST_PARSE_COUNT";
	reg_name[0x024B0>>2]="CSPREEMPT";
	reg_name[0x024B4>>2]="CTX_SEMA_REG";
	//GPGPU
	reg_name[0x02500>>2]="GPGPU_DISPATCHDIMX";
	reg_name[0x02504>>2]="GPGPU_DISPATCHDIMY";
	reg_name[0x02508>>2]="GPGPU_DISPATCHDIMZ";
	reg_name[0x02600>>2]="CS_GPR0";
	reg_name[0x02608>>2]="CS_GPR1";
	reg_name[0x02610>>2]="CS_GPR2";
	reg_name[0x02618>>2]="CS_GPR3";
	reg_name[0x02620>>2]="CS_GPR4";
	reg_name[0x02628>>2]="CS_GPR5";
	reg_name[0x02630>>2]="CS_GPR6";
	reg_name[0x02638>>2]="CS_GPR7";
	reg_name[0x02640>>2]="CS_GPR8";
	reg_name[0x02648>>2]="CS_GPR9";
	reg_name[0x02650>>2]="CS_GPR10";
	reg_name[0x02658>>2]="CS_GPR11";
	reg_name[0x02660>>2]="CS_GPR12";
	reg_name[0x02668>>2]="CS_GPR13";
	reg_name[0x02670>>2]="CS_GPR14";
	reg_name[0x02678>>2]="CS_GPR15";
	reg_name[0x02680>>2]="SEMA_REG";
	reg_name[0x02720>>2]="OASTARTTRIG5";
	reg_name[0x02740>>2]="VFSKPD";
	reg_name[0x02744>>2]="OASTARTTRIG2";
	reg_name[0x02754>>2]="OASTARTTRIG6";
	reg_name[0x02770>>2]="CEC0_0";
	reg_name[0x02778>>2]="CEC1_0";
	reg_name[0x02780>>2]="CEC2-0";
	reg_name[0x02788>>2]="CEC3-0";
	reg_name[0x0278C>>2]="OAPERF_A31";
	reg_name[0x02790>>2]="CEC4-0";
	reg_name[0x02798>>2]="CEC5-0";
	reg_name[0x027a0>>2]="CEC6-0";
	reg_name[0x027a8>>2]="CEC7-0";

	reg_name[0x02800>>2]="OAPERF_A0";
	reg_name[0x02804>>2]="OAPERF_A1";
	reg_name[0x02808>>2]="OAPERF_A2";
	reg_name[0x0280C>>2]="OAPERF_A3";
	reg_name[0x02814>>2]="OAPERF_A5";
	reg_name[0x0281C>>2]="OAPERF_A7";
	reg_name[0x02920>>2]="OAPERF_A8";
	reg_name[0x02824>>2]="OAPERF_A9";
	reg_name[0x02828>>2]="OAPERF_A10";
	reg_name[0x0282C>>2]="OAPERF_A11";
	reg_name[0x02830>>2]="OAPERF_A12";
	reg_name[0x02834>>2]="OAPERF_A13";
	reg_name[0x02838>>2]="OAPERF_A14";
	reg_name[0x0283C>>2]="OAPERF_A15";
	reg_name[0x02840>>2]="OAPERF_A16";
	reg_name[0x02844>>2]="OAPERF_A17";
	reg_name[0x02848>>2]="OAPERF_A18";
	reg_name[0x02854>>2]="OAPERF_A21";
	reg_name[0x02858>>2]="OAPERF_A22";
	reg_name[0x0285C>>2]="OAPERF_A23";
	reg_name[0x02860>>2]="OAPERF_A24";
	reg_name[0x02834>>2]="OAPERF_A25";
	reg_name[0x02868>>2]="OAPERF_A26";
	reg_name[0x0286C>>2]="OAPERF_A27";
	reg_name[0x02870>>2]="OAPERF_A28";
	reg_name[0x02874>>2]="OAPERF_A29";
	reg_name[0x02878>>2]="OAPERF_A30";
	reg_name[0x0287C>>2]="OAPERF_A31";
	//...
	reg_name[0x028B0>>2]="OAPERF_A44";
	reg_name[0x028B4>>2]="OAPERF_B0";
	reg_name[0x028B8>>2]="OAPERF_B1";
	//...
	reg_name[0x028D0>>2]="OAPERF_B7";
	
	reg_name[0x04050>>2]="ZSHR";
	reg_name[0x04060>>2]="CZWMRK";
	reg_name[0x04090>>2]="ECOCHK";
	reg_name[0x030E8>>2]="PRIV_PAT";
	reg_name[0x04204>>2]="MIDARB_PRIO_MISS_REG";
	reg_name[0x043A4>>2]="MIDARB_PRIO_NP_REG";
	reg_name[0x04208>>2]="MIDARB_PRIO_NP_REG";
	reg_name[0x043A0>>2]="MIDARB_PRIO_HIT_REG";
	reg_name[0x043A8>>2]="ARB_GAC_GAM_REQCNTS0";
	reg_name[0x043AC>>2]="ARB_GAC_GAM_REQCNTS1";
	reg_name[0x043B0>>2]="MIDARB_GOTOFIELD_HIT0";
	reg_name[0x043B4>>2]="MIDARB_GOTOFIELD_HIT1";
	reg_name[0x043B8>>2]="MIDARB_GOTOFIELD_HIT2";
	reg_name[0x043BC>>2]="MIDARB_GOTOFIELD_HIT3";
	reg_name[0x043C0>>2]="MIDARB_GOTOFIELD_NP0";
	reg_name[0x043C4>>2]="MIDARB_GOTOFIELD_NP1";
	reg_name[0x043C8>>2]="MIDARB_GOTOFIELD_NP2";
	reg_name[0x043CC>>2]="MIDARB_GOTOFIELD_NP3";
	reg_name[0x043D0>>2]="ARB_RO_GAC_GAM0";
	reg_name[0x043D4>>2]="ARB_RO_GAC_GAM1";
	reg_name[0x043D8>>2]="ARB_RO_GAC_GAM2";
	reg_name[0x043DC>>2]="ARB_RO_GAC_GAM3";
	reg_name[0x043E0>>2]="ARB_R_GAC_GAM0";
	reg_name[0x043E4>>2]="ARB_R_GAC_GAM1";
	reg_name[0x043E8>>2]="ARB_R_GAC_GAM2";
	reg_name[0x043EC>>2]="ARB_R_GAC_GAM3";
	reg_name[0x043F0>>2]="ARB_WR_GAC_GAM0";
	reg_name[0x043F4>>2]="ARB_WR_GAC_GAM1";
	reg_name[0x043F8>>2]="ARB_WR_GAC_GAM2";
	reg_name[0x043FC>>2]="ARB_WR_GAC_GAM3";
	reg_name[0x04400>>2]="TLBPEND_SEC0";
	reg_name[0x04500>>2]="INTSTATE";
	reg_name[0x04540>>2]="TLB_PEND_SEC1";//repeated
	reg_name[0x04580>>2]="PP_PFD[0:31>>2]";
	reg_name[0x04590>>2]="FAULT_SO";
	reg_name[0x04600>>2]="TLBPEND_SEC2";
	reg_name[0x04700>>2]="TLBPEND_VLD0";
	reg_name[0x04704>>2]="TLBPEND_VLD1";
	reg_name[0x04708>>2]="TLBPEND_RDY0";
	reg_name[0x0470C>>2]="TLBPEND_RDY1";
	reg_name[0x04780>>2]="MTTLB_VLD0";
	reg_name[0x04784>>2]="MTTLB_VLD1";
	reg_name[0x04788>>2]="VICTLB_VLD0";
	reg_name[0x0478C>>2]="MTVICTLB_VLD1";
	reg_name[0x04798>>2]="RCZTLB_VLD0";
	reg_name[0x0479C>>2]="RCZTLB_VLD1";
	reg_name[0x047B8>>2]="RCCTLB_VLD_0";
	reg_name[0x047BC>>2]="RCCTLB_VLD_1";
	reg_name[0x047C0>>2]="RCCTLB_VLD_2";
	reg_name[0x047C4>>2]="RCCTLB_VLD_3";
	reg_name[0x04800>>2]="MTTLB_VA";
	reg_name[0x04900>>2]="VICTLB_VA";
	reg_name[0x04A00>>2]="RCCTLB_VA";
	reg_name[0x04B00>>2]="RCZTLB_VA";
	
	reg_name[0x05200>>2]="SO_NUM_PRIMS_WRITTEN[0:3]";
	reg_name[0x05240>>2]="SO_PRIM_STORAGE_NEEDED[0:3]";
	reg_name[0x05280>>2]="SO_WRITE_OFFSET[0:3]";
	
	reg_name[0x07004>>2]="CACHE_MODE_1";
	reg_name[0x07008>>2]="GT_MODE";
	reg_name[0x07020>>2]="FBC_RT_BASE_ADDR_REG";
	reg_name[0x07028>>2]="SAMPLER_MODE";
	reg_name[0x08000>>2]="CURSOR_POS_SIGN";
	reg_name[0x0941C>>2]="GEN6_GDRST";
	reg_name[0x09420>>2]="MISCCPCTL";//ivb vol1 part6
	reg_name[0x0A00C>>2]="RC_VIDEO_FREQ";
	reg_name[0x0A168>>2]="PMINTRMSK";
	reg_name[0x0A008>>2]="GEN_6RPNSREQ]";
	//reg_name[0x0A014>>2]="GEN6_RP_INTERRUPT_LIMITS";
	reg_name[0x0A014>>2]="GEN6_RP_INTERRUPT_LIMITS";
	reg_name[0x0A024>>2]="GEN_RP_CONTROL";
	reg_name[0x0A090>>2]="RC_CONTROL";
	reg_name[0x0A094>>2]="RC_STATE";
	reg_name[0x0A098>>2]="RC1_WAKE_RATE_LIMIT";
	reg_name[0x0A09C>>2]="RC6_WAKE_RATE_LIMIT";
	reg_name[0x0A0A0>>2]="RC6pp_WAKE_RATE_LIMIT";
	reg_name[0x0A0A8>>2]="RC_EVALUATION_INTERVAL";
	reg_name[0x0A180>>2]="ECOBUS";
	reg_name[0x0A188>>2]="FORCEWAKE_MT";
	reg_name[0x0A18C>>2]="FORCEWAKE";
	reg_name[0x130090>>2]="FORCEWAKE_ACK";
	reg_name[0x0E43C>>2]="COND_DBG_VAL0";
	reg_name[0x0E448>>2]="COND_DBG_VAL1";
	reg_name[0x0E468>>2]="COND_DBG_VAL1";
	reg_name[0x0E4E4>>2]="COND_DBG_VAL3";
	reg_name[0x12028>>2]="VCS_EXCC";
	reg_name[0x12040>>2]="VBSYNC";
	reg_name[0x12044>>2]="VRSYNC";
	reg_name[0x12048>>2]="VVESYNC";
	reg_name[0x12054>>2]="VCS_PWRCTX_MAXCNT";
	reg_name[0x12098>>2]="VCS_HWSTAM";
	reg_name[0x1209C>>2]="VCS_MI_MODE";
	reg_name[0x120A0>>2]="GAC_MODE";
	reg_name[0x120A8>>2]="VCS_IMR";
	reg_name[0x120B0>>2]="VCS_EIR";
	reg_name[0x120B4>>2]="VCS_EMR";
	reg_name[0x120B8>>2]="VCS_ESR";
	reg_name[0x120C0>>2]="VCS_INSTPM";
	reg_name[0x12110>>2]="VCS_BB_STATE";
	reg_name[0x1A110>>2]="VECS_BB_STATE";
	reg_name[0x22110>>2]="BCS_BB_STATE";
	reg_name[0x02134>>2]="RCS_UHPTR";
	reg_name[0x12134>>2]="VCS_UHPTR";
	reg_name[0x22134>>2]="BCS_UHPTR";
	reg_name[0x02140>>2]="RCS_BB_ADDR";
	reg_name[0x12140>>2]="VCS_BB_ADDR";
	reg_name[0x1A140>>2]="VECS_BB_ADDR";
	reg_name[0x22140>>2]="BCS_BB_ADDR";
	reg_name[0x12144>>2]="BBA_LEVEL2";
	reg_name[0x12178>>2]="VCS_CNTR";
	reg_name[0x1217C>>2]="VCS_THRSH";
	reg_name[0x12198>>2]="VCS_RNCID";
	reg_name[0x121A8>>2]="VCS_CXT_SIZE";
	reg_name[0x12220>>2]="VCS_PP_DCLV";
	//MFX
	reg_name[0x1229C>>2]="MFX_MODE";//mfx
	reg_name[0x12358>>2]="VCS_TIMESTAMP";
	reg_name[0x12400>>2]="MFD_ERROR_STATUS";
	reg_name[0x12414>>2]="MFC_AVC_MINSIZE_PADDING_COUNT";
	reg_name[0x12420>>2]="MFD_PICTURE_PARAM";
	reg_name[0x12438>>2]="MFX_STATUS_FLAGS";
	reg_name[0x12460>>2]="MFX_FRAME_PERFORMANCE_CT";
	reg_name[0x12464>>2]="MFX_SLICE_PERFOEM_CT";
	reg_name[0x12468>>2]="MFX_MB_COUNT";
	reg_name[0x1246C>>2]="MFX_SE-BIN_CT";
	reg_name[0x12470>>2]="MFX_LAT_CT1";
	reg_name[0x12474>>2]="MFX_LAT_CT2";
	reg_name[0x12478>>2]="MFX_LAT_CT3";
	reg_name[0x1247C>>2]="MFX_LAT_CT4";
	reg_name[0x12480>>2]="MFX_ROW-PER-BS_COUNT";
	reg_name[0x12484>>2]="MFX_READ_CT";
	reg_name[0x12488>>2]="MFX_MISS_CT";
	reg_name[0x124A0>>2]="MFC_BITSTREAM_BYTECOUNT_FRAME";
	reg_name[0x124A4>>2]="MFC_BITSTREAM_SE_BITCOUNT_FRAME";
	reg_name[0x124A8>>2]="MFC_AVC_CABAC_BIN_COUNT_FRAME";
	reg_name[0x124AC>>2]="AVC_CABAC_INSERTION_COUNT";
	reg_name[0x124B4>>2]="MFC_IMAGE_STATUS_MASK";
	reg_name[0x124B8>>2]="MFC_IMAGE_STATUS_CONTROL";
	reg_name[0x124BC>>2]="MFC_QUP_CT";
	reg_name[0x124C8>>2]="VCS_PREEMPTION_HINT_UDW";
	reg_name[0x124D0>>2]="MFC_BITSTREAM_BYTECOUNT_SLICE";
	reg_name[0x124D4>>2]="MFC_BITSTREAM_SE_BITCOUNT_SLICE";
	reg_name[0x124E4>>2]="PAK_WARN";
	reg_name[0x124E8>>2]="PAK_ERR";
	reg_name[0x124EC>>2]="PAK_REPORT_STAT";
	reg_name[0x12804>>2]="MFC_VIN_AVD_ERROR_CNTR";
	reg_name[0x14040>>2]="GFX_PEND_TLB";
	reg_name[0x14050>>2]="GAC_ARB_CTL_REG";
	reg_name[0x140A0>>2]="GAC_ERROR";
	reg_name[0x14400>>2]="VCS_TLBPEND_SEC0";
	reg_name[0x14500>>2]="VCS_TLBPEND_SEC1";
	reg_name[0x14600>>2]="VCS_TLBPEND_SEC2";
	reg_name[0x14700>>2]="VCS_TLBPEND_VLD0";
	reg_name[0x14704>>2]="VCS_TLBPEND_VLD1";
	reg_name[0x14708>>2]="VCS_TLBPEND_RDY0";
	reg_name[0x1470C>>2]="VCS_TLBPEND_RDY1";
	reg_name[0x14780>>2]="MTTLB064_VLD0";
	reg_name[0x14784>>2]="MTTLB064_VLD1";
	reg_name[0x14788>>2]="MTTLB132_VLD0";
	reg_name[0x1478C>>2]="MTTLB132_VLD1";
	reg_name[0x14790>>2]="MTTLB232_VLD0";
	reg_name[0x14794>>2]="MTTLB232_VLD1";
	reg_name[0x14798>>2]="MTTLB304_VLD0";
	reg_name[0x1479C>>2]="MTTTLB304_VLD1";
	reg_name[0x14800>>2]="TLB064_VA";
	reg_name[0x14900>>2]="TLB132_VA";
	reg_name[0x14A00>>2]="TLB232_VA";
	reg_name[0x14B00>>2]="TLB304_VA";
	
	reg_name[0x1A028>>2]="VECS_EXCC";
	reg_name[0x1A040>>2]="VEBSYNC";
	reg_name[0x1A044>>2]="VEERSYNC";
	reg_name[0x1A048>>2]="VEVSYNC";
	reg_name[0x1A050>>2]="VECS_PSMI_CTRL";
	reg_name[0x1A054>>2]="VECS_PWRCTX_MAXCNT";
	//1A074
	reg_name[0x1A094>>2]="VECS_NOPID";
	reg_name[0x1A098>>2]="VECS_HWSTAM";
	reg_name[0x1A09C>>2]="VECS_MI_MODE";
	reg_name[0x1A0A8>>2]="VECS_IMR";
	reg_name[0x1A0B0>>2]="VECS_EIR";
	reg_name[0x1A0B4>>2]="VECS_EMR";
	reg_name[0x1A0B8>>2]="VECS_ESR";
	reg_name[0x1A0C0>>2]="VECS_INSTPM";
	reg_name[0x12110>>2]="VCS_BB_STATE";
	reg_name[0x1A110>>2]="VECS_BB_STATE";
	reg_name[0x22110>>2]="BCS_BB_STATE";
	reg_name[0x12134>>2]="RCS_UHPTR";
	reg_name[0x1A134>>2]="VECS_UHPTR";
	reg_name[0x22134>>2]="BCS_UHPTR";
	reg_name[0x02140>>2]="RCS_BB_ADDR";
	reg_name[0x12140>>2]="VCS_BB_ADDR";
	reg_name[0x1A140>>2]="VECS_BB_ADDR";
	reg_name[0x22140>>2]="BCS_BB_ADDR";
	reg_name[0x1A170>>2]="VECS_CTR_THRSH";
	reg_name[0x1A1A8>>2]="VECS_CXT_SIZE";
	reg_name[0x1A1D0>>2]="VECS_ECOSKPD";
	reg_name[0x1A220>>2]="VECS_PP_DCLV";
	reg_name[0x1A23C>>2]="VECS_IDLEDLY";
	reg_name[0x1A29C>>2]="VEBOX_MODE";
	reg_name[0x1A358>>2]="VECS_TIMESTAMP";
	
	reg_name[0x22028>>2]="BCS_EXCC";
	reg_name[0x22030>>2]="DOC_UNKNOWN";
	reg_name[0x22040>>2]="BRSYNC";
	reg_name[0x22044>>2]="BVSYNC";
	reg_name[0x22048>>2]="BVESYNC";
	reg_name[0x22050>>2]="BCS_PSMI_CTRL";
	reg_name[0x22054>>2]="BCS_PWRCTX_MAXCNT";
	//reg_name[0x1A094>>2]="VECS_NOPID";
	reg_name[0x22098>>2]="VCS_HWSTAM";
	reg_name[0x2209C>>2]="BCS_MI_MODE";
	reg_name[0x220A0>>2]="GAB_MODE";
	reg_name[0x220A8>>2]="BCS_IMR";
	reg_name[0x220B0>>2]="BCS_EIR";
	reg_name[0x220B4>>2]="BCS_EMR";
	reg_name[0x220B8>>2]="BCS_ESR";
	reg_name[0x220C0>>2]="BCS_INSTPM";
	reg_name[0x22170>>2]="BCS_CTR_THRSH";
	reg_name[0x22190>>2]="BCS_RCCID";
	reg_name[0x22198>>2]="BCS_RNCID";
	reg_name[0x22200>>2]="BCS_SWCTRL";
	reg_name[0x22220>>2]="BCS_PP_DCLV";
	reg_name[0x2229C>>2]="BLT_MODE";
	reg_name[0x22358>>2]="bCS_TIMESTAMP";
	reg_name[0x24000>>2]="GAB_CTL_REG";
	reg_name[0x24094>>2]="GAB_ERR_REPORT";
	reg_name[0x24400>>2]="BCS_TLBPEND_SEC0";
	reg_name[0x24500>>2]="BCS_TLBPEND_SEC1";
	reg_name[0x24700>>2]="BCS_TLBPEND_VLD0";
	reg_name[0x24708>>2]="BCS_TLBPEND_RDY0";
	reg_name[0x24780>>2]="BCSTLB_VLD";
	reg_name[0x24784>>2]="BLBTLB_VLD";
	reg_name[0x24788>>2]="CTX_TLB_VLD";
	reg_name[0x2478C>>2]="PDTLB_VLD";
	reg_name[0x24800>>2]="BCSTLB_VA";
	reg_name[0x24900>>2]="BLBTLB_VA";
	reg_name[0x24A00>>2]="CTXTLB_VA";
	reg_name[0x24B00>>2]="PDTLB_VA";

	reg_name[0x41000>>2]="VGA_CONTROL";
	reg_name[0x42080>>2]="CHICKEN_PAR1_1";
	reg_name[0x42014>>2]="FUSE_STRAP";
	reg_name[0x4201C>>2]="FUSE_STRAP2";
	reg_name[0x42020>>2]="FUSE_STRAP3";
	reg_name[0x42024>>2]="FUSE_STRAP4";
	reg_name[0x42300>>2]="FPGA_DBG";
	omitted[0x42300]=0xff;
	reg_name[0x42400>>2]="DE_POWER1";
	reg_name[0x42404>>2]="DE_POWER2";
	reg_name[0x43200>>2]="FBC_CFB_BASE";
	reg_name[0x43208>>2]="FBC_CTL";
	reg_name[0x43408>>2]="IPS_CTL";
	reg_name[0x43410>>2]="IPS_STATUS";
	reg_name[0x44000>>2]="DE_INTERRUPT";//display engine
	reg_name[0x44004>>2]="DE_IMR";
	reg_name[0x44008>>2]="DE_IIR";
	
	reg_name[0x4400C>>2]="DE_IER";
	memset(&omitted[0x44000], 0xFF, 5);
	omitted[0x44008] = 0xFF;
	omitted[0x4400C] = 0xFF;
	reg_name[0x44010>>2]="GT_INTERRUPT";
	reg_name[0x44014>>2]="GT_IMR";
	reg_name[0x44018>>2]="GT_IIR";
	omitted[0x44018]=0xFF;
	reg_name[0x4401C>>2]="GT_IER";
	reg_name[0x44020>>2]="PM_INTERRUPT";
	reg_name[0x44024>>2]="PM_IMR";
	reg_name[0x44028>>2]="PM_IIR";
	omitted[0x44028]=0xFF;
	reg_name[0x4402C>>2]="PM_IER";
	reg_name[0x44030>>2]="HOTPLUG_CTL";
	reg_name[0x44034>>2]="HPD_PULSE_CNT";
	reg_name[0x44038>>2]="HPD_FILTER_CNT";
	reg_name[0x44040>>2]="ERR_INT";
	reg_name[0x44050>>2]="DE_RRMR";
	reg_name[0x44070>>2]="TIMESTAMP_CTR";
	omitted[0x44070]=0xFF;
	reg_name[0x44080>>2]="AUD_INTERRUPT";
	reg_name[0x45000>>2]="ARB_CTL";
	reg_name[0x45004>>2]="ARB_CTL2";
	reg_name[0x45100>>2]="WM_PIPE_A";
	reg_name[0x45104>>2]="WM_PIPE_B";
	reg_name[0x45200>>2]="WM_PIPE_C";
	reg_name[0x45108>>2]="WM_LP";
	reg_name[0x4510C>>2]="WM_LP2";
	reg_name[0x45110>>2]="WM_LP3";
	reg_name[0x45120>>2]="WM_LP1_SPR";
	reg_name[0x45124>>2]="WM_LP2_SPR";
	reg_name[0x45128>>2]="WM_LP3_SPR";
	reg_name[0x45260>>2]="WM_MISC";
	reg_name[0x45270>>2]="WM_LINETIME_A";
	reg_name[0x45274>>2]="WM_LINETIME_B";
	reg_name[0x45278>>2]="WM_LINETIME_C";
	
	reg_name[0x45400>>2]="PWR_WELL_CTL1";
	reg_name[0x45404>>2]="PWR_WELL_CTL2";
	reg_name[0x46020>>2]="SPLL_CTL";
	reg_name[0x46040>>2]="WRPLL_CTL1";
	reg_name[0x46060>>2]="WRPLL_CTL2";
	reg_name[0x46100>>2]="PORT_CLK_SEL_DDIA";
	reg_name[0x46104>>2]="PORT_CLK_SEL_DDIB";
	reg_name[0x46108>>2]="PORT_CLK_SEL_DDIC";
	reg_name[0x4610C>>2]="PORT_CLK_SEL_DDID";
	reg_name[0x46110>>2]="PORT_CLK_SEL_DDIE";
	reg_name[0x46140>>2]="TRANS_CLK_SEL_A";
	reg_name[0x46200>>2]="CDCLK_REQ";
	reg_name[0x46408>>2]="NDE_RSTWRN_OPT";
	reg_name[0x48250>>2]="BLC_WPM_CTL";
	reg_name[0x48250>>2]="BLC_WPM_DATA";
	reg_name[0x48260>>2]="BLM_HIST_CTL";
	reg_name[0x48264>>2]="BLM_HIST_BIN";
	reg_name[0x48268>>2]="BLM_HIST_GUARD";
	reg_name[0x48350>>2]="BLC_PWM2_CTL";
	reg_name[0x48254>>2]="BLC_PWM_DATA";
	reg_name[0x48354>>2]="BLC_PWM2_DATA";
	reg_name[0x46360>>2]="BLC_MISC_CTL";
	reg_name[0x48400>>2]="UTIL_PIN_CTL";
	reg_name[0x49010>>2]="CSC_COEFF_A_24bytes";
	memset(&omitted[0x49010], 0xff, 24);
	reg_name[0x49110>>2]="CSC_CPEFF_B";
	memset(&omitted[0x49110], 0xff, 24);
	reg_name[0x49028>>2]="CSC_MODE_A";
	reg_name[0x49128>>2]="CSC_MODE_B";
	reg_name[0x49228>>2]="CSC_MODE_C";
	reg_name[0x49030>>2]="CSC_PREOFF_A";
	reg_name[0x49130>>2]="CSC_PREOFF_B";
	reg_name[0x49230>>2]="CSC_PREOFF_C";
	reg_name[0x49040>>2]="CSC_POSTOFF_A";
	reg_name[0x49140>>2]="CSC_POSTOFF_B";
	reg_name[0x49240>>2]="CSC_POSTOFF_c";
	reg_name[0x49080>>2]="CGE_CTRL_A";
	reg_name[0x49180>>2]="CGE_CTRL_B";
	reg_name[0x49280>>2]="CGE_CTRL_C";
	reg_name[0x49090>>2]="CGE_WEIGHT_A";
	reg_name[0x49190>>2]="CGE_WEIGHT_B";
	reg_name[0x49290>>2]="CGE_WEIGHT_C";
	reg_name[0x4a000>>2]="PAL_LGC_A SIZE 0x3FF";//PALETTE
	memset(&omitted[0x4a004], 0xff, 0x3fe);
	reg_name[0x4A800>>2]="PAL_LGC_B";
	reg_name[0x4B000>>2]="PAL_LGC_c";
	reg_name[0x4A400>>2]="PAL_PREC_INDEX_A";
	reg_name[0x4AC00>>2]="PAL_PREC_INDEX_B";
	reg_name[0x4B400>>2]="PAL_PREC_INDEX_C";
	reg_name[0x4A404>>2]="PAL_PREC_DATA_A";
	reg_name[0x4AC04>>2]="PAL_PREC_DATA_B";
	reg_name[0x4B404>>2]="PAL_PREC_DATA_C";
	reg_name[0x4A410>>2]="PAL_GC_MAX_A";
	reg_name[0x4AC10>>2]="PAL_GC_MAX_B";
	reg_name[0x4B410>>2]="PAL_GC_MAX_C";
	reg_name[0x4A420>>2]="PAL_EXT_GC_MAX_A";
	reg_name[0x4a480>>2]="_GAMMA_MODE_A";
	reg_name[0x4AC20>>2]="PAL_EXT_GC_MAX_B";
	reg_name[0x4B420>>2]="PAL_EXT_GC_MAX_C";
	reg_name[0x4F000>>2]="SWF_*";
	reg_name[0x4F044>>2]="DOCUMENTED_UNKNOWN";
	reg_name[0x4f05c>>2]="DOCUMENTED_UNKNOWN";
	reg_name[0x4F060>>2]="SWF_";
	//...
	reg_name[0x4F100>>2]="GTSCRATCH_*";
	//...
	reg_name[0x60000>>2]="PIPE_HTOTAL_A";
	reg_name[0x6000C>>2]="_VTOTAL_A";
	reg_name[0x61000>>2]="PIPE_HTOTAL_B";
	reg_name[0x62000>>2]="PIPE_HTOTAL_C";
	reg_name[0x6F000>>2]="PIPE_HTOTAL_EDP";
	reg_name[0x60004>>2]="PIPE_HBLANK_A";
	reg_name[0x61004>>2]="PIPE_HBLANK_B";
	reg_name[0x62004>>2]="PIPE_HBLANK_C";
	reg_name[0x6F004>>2]="PIPE_HBLANK_EDP";
	reg_name[0x60008>>2]="PIPE_HSYNC_A";
	reg_name[0x61008>>2]="PIPE_HSYNC_B";
	reg_name[0x62008>>2]="PIPE_HSYNC_C";
	reg_name[0x6F008>>2]="PIPE_HSYNC_EDP";
	reg_name[0x6100C>>2]="PIPE_VTOTAL";;
	reg_name[0x6100C>>2]="PIPE_VTOTAL_B";
	reg_name[0x6200C>>2]="PIPE_VTOTAL_C";
	reg_name[0x6F00C>>2]="PIPE_VTOTAL_EDP";
	reg_name[0x60010>>2]="PIPE_VBLANK_A";
	reg_name[0x61010>>2]="PIPE_VBLANK_B";
	reg_name[0x62010>>2]="PIPE_VBLANK_C";
	reg_name[0x6F010>>2]="PIPE_VBLANK_EDP";
	reg_name[0x60014>>2]="PIPE_VSYNC_A";
	reg_name[0x61014>>2]="PIPE_VSYNC_B";
	reg_name[0x62014>>2]="PIPE_VSYNC_C";
	reg_name[0x6F014>>2]="PIPE_VSYNC_EDP";
	reg_name[0x6001C>>2]="PIPE_SRCSZ_A";
	reg_name[0x6101C>>2]="PIPE_SRCSZ_B";
	reg_name[0x6201C>>2]="PIPE_SRCSZ_c";
	reg_name[0x60028>>2]="PIPE_VSYNCSHIFT_A";
	reg_name[0x61028>>2]="PIPE_VSYNCSHIFT_B";
	reg_name[0x62028>>2]="PIPE_VSYNCSHIFT_C";
	reg_name[0x6F028>>2]="PIPE_VSYNCSHIFT_EDP";
	reg_name[0x6002C>>2]="PIPE_MULT_A";
	reg_name[0x6102C>>2]="PIPE_MULT_B";
	reg_name[0x6202C>>2]="PIPE_MULT_C";
	reg_name[0x60030>>2]="PIPE_DATAM1_A";
	reg_name[0x61030>>2]="PIPE_DATAM1_B";
	reg_name[0x62030>>2]="PIPE_DATAM1_C";
	reg_name[0x6F030>>2]="PIPE_DATAM1_EDP";
	reg_name[0x6F038>>2]="PIPE_DATAM2_EDP";
	reg_name[0x60034>>2]="PIPE_DATAN1_A";
	reg_name[0x61034>>2]="PIPE_DATAN1_B";
	reg_name[0x62034>>2]="PIPE_DATAN1_C";
	reg_name[0x6F034>>2]="PIPE_DATAN1_EDP";
	reg_name[0x60040>>2]="PIPE_LINKM1_A";
	reg_name[0x61040>>2]="PIPE_LINKM1_B";
	reg_name[0x62040>>2]="PIPE_LINKM1_C";
	reg_name[0x6F040>>2]="PIPE_LINKM1_EDP";
	reg_name[0x6F048>>2]="PIPE_LINKM2_EDP";
	reg_name[0x60044>>2]="PIPE_LINKN1_A";
	reg_name[0x61044>>2]="PIPE_LINKN1_B";
	reg_name[0x62044>>2]="PIPE_LINKN1_C";
	reg_name[0x6F044>>2]="PIPE_LINKN1_EDP";
	reg_name[0x6F04C>>2]="PIPE_LINKN2_EDP";
	reg_name[0x60200>>2]="VIDEO_DIP_CTL_A";
	reg_name[0x61200>>2]="VIDEO_DIP_CTL_B";
	reg_name[0x62200>>2]="VIDEO_DIP_CTL_C";
	reg_name[0x6F200>>2]="VIDEO_DIP_CTL_EDP";
	reg_name[0x60210>>2]="VIDEO_DIP_GCP_A";
	reg_name[0x61210>>2]="VIDEO_DIP_GCP_B";
	reg_name[0x62210>>2]="VIDEO_DIP_GCP_C";
	reg_name[0x60220>>2]="VIDEO_DIP_AVI_DATA_A_*";
	reg_name[0x60260>>2]="VIDEO_DIP_VS_DATA_A_*";
	reg_name[0x602A0>>2]="VIDEO_DIP_SPD_DATA_A_*";
	reg_name[0x602E0>>2]="VIDEO_DIP_GMP_DATA_A_*";
	reg_name[0x60320>>2]="VIDEO_DIP_VSC_DATA_A_*";
	reg_name[0x61110>>2]="PORT_HOTPLUG_EN";
	reg_name[0x61220>>2]="VIDEO_DIP_AVI_DATA_B_*";
	reg_name[0x61260>>2]="VIDEO_DIP_VS_DATA_B_*";
	reg_name[0x612A0>>2]="VIDEO_DIP_SPD_DATA_B_*";
	reg_name[0x612E0>>2]="VIDEO_DIP_GMP_DATA_B_*";
	reg_name[0x61320>>2]="VIDEO_DIP_VSC_DATA_B_*";
	reg_name[0x62220>>2]="VIDEO_DIP_AVI_DATA_C_*";
	reg_name[0x62260>>2]="VIDEO_DIP_VS_DATA_C_*";
	reg_name[0x622A0>>2]="VIDEO_DIP_SPD_DATA_C_*";
	reg_name[0x622E0>>2]="VIDEO_DIP_GMP_DATA_C_*";
	reg_name[0x62320>>2]="VIDEO_DIP_VSC_DATA_C_*";
	reg_name[0x64F08>>2]="DOC_UNKNOWN";
	reg_name[0x64F0C>>2]="DOC_UNKNOWN";
	reg_name[0x64F68>>2]="DOC_UNKNOWN";
	reg_name[0x64F6C>>2]="DOC_UNKNOWN";
	reg_name[0x6F320>>2]="VIDEO_DIP_VSC_DATA_EDP_*";
	reg_name[0x60240>>2]="VIDEO_DIP_AVI_ECC_A_*";
	reg_name[0x60280>>2]="VIDEO_DIP_VS_ECC_A_*";
	reg_name[0x602C0>>2]="VIDEO_DIP_SPD_ECC_A_*";
	reg_name[0x60300>>2]="VIDEO_DIP_GMP_ECC_A_*";
	reg_name[0x60344>>2]="VIDEO_DIP_VSC_ECC_A_*";
	reg_name[0x61240>>2]="VIDEO_DIP_AVI_ECC_B_*";
	reg_name[0x61280>>2]="VIDEO_DIP_VS_ECC_B_*";
	reg_name[0x612C0>>2]="VIDEO_DIP_SPD_ECC_B_*";
	reg_name[0x61300>>2]="VIDEO_DIP_GMP_ECC_B_*";
	reg_name[0x61344>>2]="VIDEO_DIP_VSC_ECC_B_*";
	reg_name[0x62240>>2]="VIDEO_DIP_AVI_ECC_C_*";
	reg_name[0x62280>>2]="VIDEO_DIP_VS_ECC_C_*";
	reg_name[0x622C0>>2]="VIDEO_DIP_SPD_ECC_C_*";
	reg_name[0x62300>>2]="VIDEO_DIP_GMP_ECC_C_*";
	reg_name[0x62344>>2]="VIDEO_DIP_VSC_ECC_C_*";
	reg_name[0x6F344>>2]="VIDEO_DIP_VSC_ECC_EDP_*";
	reg_name[0x60400>>2]="PIPE_DDI_FUNC_CTL_A";
	reg_name[0x61400>>2]="PIPE_DDI_FUNC_CTL_B";
	reg_name[0x62400>>2]="PIPE_DDI_FUNC_CTL_C";
	reg_name[0x6F400>>2]="PIPE_DDI_FUNC_CTL_EDP";\
	reg_name[0x60410>>2]="PIPE_MSA_MISC_A";
	reg_name[0x61410>>2]="PIPE_MSA_MISC_B";
	reg_name[0x62410>>2]="PIPE_MSA_MISC_C";
	reg_name[0x6F410>>2]="PIPE_MSA_MISC_EDP";//DUPLICATE AT PAGE 612
	reg_name[0x64000>>2]="DDI_BUF_CTL_A";
	reg_name[0x64100>>2]="DDI_BUF_CTL_B";
	reg_name[0x64200>>2]="DDI_BUF_CTL_C";
	reg_name[0x64300>>2]="DDI_BUF_CTL_D";
	reg_name[0x64400>>2]="DDI_BUF_CTL_E";
	reg_name[0x64010>>2]="DDI_AUX_CTL_A";
	reg_name[0x64014>>2]="DDI_AUX_DATA_A_*";
	reg_name[0x64040>>2]="DDI_TP_CTL_A";
	reg_name[0x64140>>2]="DDI_TP_CTL_B";
	reg_name[0x64240>>2]="DDI_TP_CTL_C";
	reg_name[0x64340>>2]="DDI_TP_CTL_D";
	reg_name[0x64440>>2]="DDI_TP_CTL_E";
	reg_name[0x64044>>2]="DDI_DP_TP_STATUS_A";
	reg_name[0x64144>>2]="DDI_DP_TP_STATUS_B";
	reg_name[0x64244>>2]="DDI_DP_TP_STATUS_C";
	reg_name[0x64344>>2]="DDI_DP_TP_STATUS_D";
	reg_name[0x64444>>2]="DDI_DP_TP_STATUS_E";
	reg_name[0x64800>>2]="SRD_CTL";//PANEL SELF REFRESH
	reg_name[0x64810>>2]="SRD_AUX_CTL";
	reg_name[0x64814>>2]="SRD_AUX_DATA_*";//5 INSTANCE
	reg_name[0x64834>>2]="SRD_IMR";
	reg_name[0x64838>>2]="SRD_IIR";
	reg_name[0x64840>>2]="SRD_STATUS";
	reg_name[0x64E00>>2]="DDI_BUF_TRANS_A";
	memset(&omitted[0x64E04], 0xFF, 19);
	reg_name[0x64E60>>2]="DDI_BUF_TRANS_B";
	memset(&omitted[0x64E64], 0xFF, 19);
	reg_name[0x64EC0>>2]="DDI_BUF_TRANS_C";
	memset(&omitted[0x64EC4], 0xFF, 19);
	reg_name[0x64F20>>2]="DDI_BUF_TRANS_D";
	memset(&omitted[0x64F24], 0xFF, 19);
	reg_name[0x64F80>>2]="DDI_BUF_TRANS_E";
	memset(&omitted[0x64F84], 0xFF, 19);
	reg_name[0x65000>>2]="AUD_TCA_CONFIG";
	reg_name[0x65100>>2]="AUD_TCB_CONFIG";
	reg_name[0x65200>>2]="AUD_TCC_CONFIG";
	reg_name[0x65010>>2]="AUD_C1_MISC_CTRL";
	reg_name[0x65110>>2]="AUD_C2_MISC_CTRL";
	reg_name[0x65210>>2]="AUD_C3_MISC_CTRL";
	reg_name[0x65020>>2]="AUD_VID_DID";
	reg_name[0x60024>>2]="AUD_RID";
	reg_name[0x65028>>2]="AUD_TCA_M_CTS_ENABLE";//AUDIO TRANSCODER
	reg_name[0x65128>>2]="AUD_TCB_M_CTS_ENABLE";
	reg_name[0x65228>>2]="AUD_TCC_M_CTS_ENABLE";
	reg_name[0x6504C>>2]="AUD_PWRST";
	reg_name[0x65050>>2]="AUD_TCA_EDID_DATA";
	reg_name[0x65150>>2]="AUD_TCB_EDID_DATA";
	reg_name[0x65250>>2]="AUD_TCC_EDID_DATA";
	reg_name[0x65054>>2]="AUD_TCA_INFOFR";
	reg_name[0x65154>>2]="AUD_TCB_INFOFR";
	reg_name[0x65254>>2]="AUD_TCC_INFOFR";
	reg_name[0x650A8>>2]="AUD_TCA_PIN_PIPE_CONN_ENTRY_LNGTH_RO";
	reg_name[0x651A8>>2]="AUD_TCB_PIN_PIPE_CONN_ENTRY_LNGTH_RO";
	reg_name[0x652A8>>2]="AUD_TCC_PIN_PIPE_CONN_ENTRY_LNGTH_RO";
	reg_name[0x650AC>>2]="AUD_PIPE_CONN_SEL_CTRL";
	reg_name[0x650B4>>2]="AUD_TCA_DIP_ELD_CTRL_ST";
	reg_name[0x651B4>>2]="AUD_TCB_DIP_ELD_CTRL_ST";
	reg_name[0x652B4>>2]="AUD_TCC_DIP_ELD_CTRL_ST";
	reg_name[0x650C0>>2]="AUD_PIN_ELD_CP_VLD";
	reg_name[0x65f10>>2]="DOC_UNKNOWN";

	reg_name[0x67000>>2]="GTC_CPU_CTL";
	reg_name[0x67004>>2]="GTC_CPU_MISC";
	reg_name[0x67010>>2]="GTC_CPU_DDA_M";
	reg_name[0x67014>>2]="GTC_CPU_DDA_N";
	reg_name[0x67020>>2]="GTC_CPU_LIVE";
	reg_name[0x67024>>2]="GTC_CPU_REMOTE_CURR";
	reg_name[0x67028>>2]="GTC_CPU_LOCAL_CURR";
	reg_name[0x6702C>>2]="GTC_CPU_REMOTE_PREV";
	reg_name[0x67030>>2]="GTC_CPU_LOCAL_PREV";
	reg_name[0x67054>>2]="GTC_CPU_IMR";
	reg_name[0x67058>>2]="GTC_CPU_IIR";
	reg_name[0x68060>>2]="PF_PWR_GATE_0";
	reg_name[0x68860>>2]="PF_PWR_GATE_1";
	reg_name[0x69060>>2]="PF_PWR_GATE_2";
	reg_name[0x68070>>2]="PF_WIN_POS_0";
	reg_name[0x68870>>2]="PF_WIN_POS_1";
	reg_name[0x69070>>2]="PF_WIN_POS_2";
	reg_name[0x68074>>2]="PF_WIN_SZ_0";
	reg_name[0x68874>>2]="PF_WIN_SZ_1";
	reg_name[0x69074>>2]="PF_WIN_SZ_2";
	reg_name[0x68080>>2]="PF_CTRL_0";
	reg_name[0x68880>>2]="PF_CTRL_1";
	reg_name[0x69080>>2]="PF_CTRL_2";
	reg_name[0x6C01C>>2]="DISPIO_CR_TX_BMU_CR4";
	reg_name[0x60410>>2]="PIPE_MSA_MISC_A";
	reg_name[0x61410>>2]="PIPE_MSA_MISC_B";
	reg_name[0x62410>>2]="PIPE_MSA_MISC_C";
	reg_name[0x6F410>>2]="PIPE_MSA_MISC_EDPC";
	reg_name[0x70000>>2]="PIPE_SCANLINE_A";
	reg_name[0x71000>>2]="PIPE_SCANLINE_B";
	reg_name[0x72000>>2]="PIPE_SCANLINE_c";
	reg_name[0x70004>>2]="PIPE_SCANLINECOMP_A";
	reg_name[0x71004>>2]="PIPE_SCANLINECOMP_B";
	reg_name[0x72004>>2]="PIPE_SCANLINECOMP_c";
	reg_name[0x70008>>2]="PIPE_CONF_A";
	omitted[0x70008]=0xff;
	reg_name[0x71008>>2]="PIPE_CONF_B";
	reg_name[0x72008>>2]="PIPE_CONF_C";
	reg_name[0x7F008>>2]="PIPE_CONF_EDP";
	reg_name[0x70040>>2]="PIPE_FRMCNT_A";
	omitted[0x70040]=0xff;
	reg_name[0x71040>>2]="PIPE_FRMCNT_B";
	reg_name[0x72040>>2]="PIPE_FRMCNT_c";
	reg_name[0x70044>>2]="PIPE_FLIPCNT_A";
	reg_name[0x71044>>2]="PIPE_FLIPCNT_B";
	reg_name[0x72044>>2]="PIPE_FLIPCNT_C";
	reg_name[0x70048>>2]="PIPE_TIMESTAMP_A";
	reg_name[0x71048>>2]="PIPE_TIMESTAMP_B";
	reg_name[0x72048>>2]="PIPE_TIMESTAMP_C";
	reg_name[0x7004C>>2]="PIPE_FLIPTMSTMP_A";
	reg_name[0x7104C>>2]="PIPE_FLIPTMSTMP_B";
	reg_name[0x7204C>>2]="PIPE_FLIPTMSTMP_C";
	reg_name[0x70080>>2]="CUR_CTL_A";
	reg_name[0x71080>>2]="CUR_CTL_B";
	reg_name[0x72080>>2]="CUR_CTL_C";
	reg_name[0x70084>>2]="CUR_BASE_A";
	omitted[0x70084] = 0xff;
	reg_name[0x71084>>2]="CUR_BASE_B";
	reg_name[0x72084>>2]="CUR_BASE_C";
	reg_name[0x70088>>2]="CUR_POS_A";
	omitted[0x70088] = 0xff;
	reg_name[0x71088>>2]="CUR_POS_B";
	reg_name[0x72088>>2]="CUR_POS_C";
	reg_name[0x70090>>2]="CUR_PAL_A";
	reg_name[0x71090>>2]="CUR_PAL_B";
	reg_name[0x72090>>2]="CUR_PAL_C";
	reg_name[0x700A0>>2]="CUR_FBC_CTL_A";
	reg_name[0x710A0>>2]="CUR_FBC_CTL_B";
	reg_name[0x720A0>>2]="CUR_FBC_CTL_C";
	reg_name[0x700AC>>2]="CUR_SURFLIVE_A";
	reg_name[0x701AC>>2]="PRI_SURFLIVE_A";
	omitted[0x701ac]=0xFF;
	reg_name[0x701BC>>2]="PRI_LEFT_SURFLIVE_A";
	reg_name[0x702AC>>2]="SPR_SURFLIVE_A";
	reg_name[0x702BC>>2]="SPR_LEFT_SURFLIVE_A";
	reg_name[0x710AC>>2]="CUR_SURFLIVE_B";
	reg_name[0x711AC>>2]="PRI_SURFLIVE_B";
	reg_name[0x711BC>>2]="PRI_LEFT_SURFLIVE_B";
	reg_name[0x712AC>>2]="SPR_SURFLIVE_B";
	reg_name[0x712BC>>2]="SPR_LEFT_SURFLIVE_B";
	reg_name[0x720AC>>2]="CUR_SURFLIVE_C";
	reg_name[0x721AC>>2]="PRI_SURFLIVE_C";
	reg_name[0x721BC>>2]="PRI_LEFT_SURFLIVE_C";
	reg_name[0x722AC>>2]="SPR_SURFLIVE_C";
	reg_name[0x722BC>>2]="SPR_LEFT_SURFLIVE_AC";
	reg_name[0x70180>>2]="PRI_CTL_A";
	omitted[0x070180]=0xFF;
	reg_name[0x71180>>2]="PRI_CTL_B";
	reg_name[0x72180>>2]="PRI_CTL_C";
	reg_name[0x70188>>2]="PRI_STRIDE_A";
	reg_name[0x71188>>2]="PRI_STRIDE_B";
	reg_name[0x72188>>2]="PRI_STRIDE_C";
	reg_name[0x7019C>>2]="PRI_SURF_A";
	omitted[0x7019C]=0xFF;
	reg_name[0x7119C>>2]="PRI_SURF_B";
	reg_name[0x7219C>>2]="PRI_SURF_C";
	reg_name[0x701A4>>2]="PRI_OFFSET_A";
	omitted[0x701A4]=0xFF;
	reg_name[0x711A4>>2]="PRI_OFFSET_B";
	reg_name[0x721A4>>2]="PRI_OFFSET_C";
	reg_name[0x701B0>>2]="PRI_LEFT_SURF_A";
	reg_name[0x711B0>>2]="PRI_LEFT_SURF_B";
	reg_name[0x721B0>>2]="PRI_LEFT_SURF_C";
	reg_name[0x70280>>2]="SPR_CTL_A";
	reg_name[0x71280>>2]="SPR_CTL_B";
	reg_name[0x72280>>2]="SPR_CTL_C";
	reg_name[0x70288>>2]="SPR_STRIDE_A";
	reg_name[0x71288>>2]="SPR_STRIDE_B";
	reg_name[0x72288>>2]="SPR_STRIDE_C";
	reg_name[0x7028C>>2]="SPR_POS_A";
	reg_name[0x7128C>>2]="SPR_POS_B";
	reg_name[0x7228C>>2]="SPR_POS_C";
	reg_name[0x70290>>2]="SPR_SIZE_A";
	reg_name[0x71290>>2]="SPR_SIZE_B";
	reg_name[0x72290>>2]="SPR_SIZE_C";
	reg_name[0x70294>>2]="SPR_KEYVAL_A";
	reg_name[0x71294>>2]="SPR_KEYVAL_B";
	reg_name[0x72294>>2]="SPR_KEYVAL_C";
	reg_name[0x70298>>2]="SPR_KEYMSK_A";
	reg_name[0x71298>>2]="SPR_KEYMSK_B";
	reg_name[0x72298>>2]="SPR_KEYMSK_C";
	reg_name[0x7029C>>2]="SPR_SURF_A";
	reg_name[0x7129C>>2]="SPR_SURF_B";
	reg_name[0x7229C>>2]="SPR_SURF_C";
	reg_name[0x702A0>>2]="SPR_KEYMAX_A";
	reg_name[0x712A0>>2]="SPR_KEYMAX_B";
	reg_name[0x722A0>>2]="SPR_KEYMAX_C";
	reg_name[0x702A4>>2]="SPR_OFFSET_A";
	reg_name[0x712A4>>2]="SPR_OFFSET_B";
	reg_name[0x722A4>>2]="SPR_OFFSET_C";
	reg_name[0x702B0>>2]="SPR_LEFT_SURF_A";
	reg_name[0x712B0>>2]="SPR_LEFT_SURF_B";
	reg_name[0x722B0>>2]="SPR_LEFT_SURF_C";
	reg_name[0x70400>>2]="SPR_GAMC_A";
	reg_name[0x71400>>2]="SPR_GAMC_B";
	reg_name[0x72400>>2]="SPR_GAMC_C";
	reg_name[0x70440>>2]="SPR_GAMC16_A";
	reg_name[0x71440>>2]="SPR_GAMC16_B";
	reg_name[0x72440>>2]="SPR_GAMC16_C";
	reg_name[0x7044C>>2]="SPR_GAMC17_A";
	reg_name[0x7144C>>2]="SPR_GAMC17_B";
	reg_name[0x7244C>>2]="SPR_GAMC17_C";
	reg_name[0xc2004>>2]="SOUTH_CHICKEN_2";
	reg_name[0xC2014>>2]="SFUSE_STRAP";
	reg_name[0xC4000>>2]="SDE_INTERRUPT";
	reg_name[0xC4004>>2]="SDE_IMR";
	reg_name[0xC4008>>2]="SDE_IIR";
	reg_name[0xC400C>>2]="SDE_IER";
	reg_name[0xC4030>>2]="SHOTPLUG_CTL";
	reg_name[0xC4034>>2]="SHPD_PULSE_CNT";
	reg_name[0xC4044>>2]="SHPD_PULSE_CNT_C";
	reg_name[0xC4048>>2]="SHPD_PULSE_CNT_D";
	reg_name[0xC4038>>2]="SHPD_FILTER_CNT";
	reg_name[0xC4040>>2]="SERR_INT";
	reg_name[0xC5010>>2]="GPIO_CTL_0";
	reg_name[0xC501C>>2]="GPIO_CTL_3";
	reg_name[0xC5020>>2]="GPIO_CTL_4";
	reg_name[0xC5024>>2]="GPIO_CTL_5";
	reg_name[0xC5100>>2]="GMBUS0";
	reg_name[0xC5104>>2]="GMBUS1";
	reg_name[0xC5108>>2]="GMBUS2";
	reg_name[0xC510C>>2]="GMBUS3";
	reg_name[0xC5110>>2]="GMBUS4";
	reg_name[0xC5120>>2]="GMBUS5";
	reg_name[0xC6000>>2]="SBI_ADDR";
	reg_name[0xC6004>>2]="SBI_DATA";
	reg_name[0xC6008>>2]="SBI_CTL_STAT";
	reg_name[0xC6020>>2]="PIXCLK_GATE";
	reg_name[0xC6030>>2]="GTCCLK_EN";
	reg_name[0xC6204>>2]="RAWCLK_FREQ";
	reg_name[0xC7200>>2]="PP_STATUS";
	reg_name[0xC7204>>2]="PP_CONTROL";
	reg_name[0xC7208>>2]="PP_ON_DELAYS";
	reg_name[0xC720C>>2]="PP_OFF_DELAYS";
	reg_name[0xC7210>>2]="PP_DIVISOR";
	reg_name[0xC8250>>2]="SBLC_PWM_CTL1";
	reg_name[0xC8254>>2]="SBLC_PWM_CTL2";
	reg_name[0xE0000>>2]="TRANS_HTOTAL_A";
	reg_name[0xE0004>>2]="TRANS_HBLANK_A";
	reg_name[0xE0008>>2]="TRANS_HSYNC_A";
	reg_name[0xE000C>>2]="TRANS_VTOTAL_A";
	reg_name[0xE0010>>2]="TRANS_VBLANK_A";
	reg_name[0xE0014>>2]="TRANS_VSYNC_A";
	reg_name[0xE0028>>2]="TRANS_VSYNCSHIFT_A";
	reg_name[0xE1100>>2]="DAC_CTL";//CRT DAC
	reg_name[0xE4110>>2]="DP_AUX_CTL_B";
	reg_name[0xE4210>>2]="DP_AUX_CTL_C";
	reg_name[0xE4310>>2]="DP_AUX_CTL_D";
	reg_name[0xE4114>>2]="DP_AUX_DATA_B";
	reg_name[0xE4214>>2]="DP_AUX_DATA_C";
	reg_name[0xE4314>>2]="DP_AUX_DATA_D";
	reg_name[0xE7000>>2]="GTC_CTL";
	reg_name[0xE7004>>2]="GTC_MISC";
	reg_name[0xE7010>>2]="GTC_DDA_M";
	reg_name[0xE7014>>2]="GTC_DDA_N";
	reg_name[0xE7054>>2]="GTC_PCH_IMR";
	reg_name[0xE7058>>2]="GTC_PCH_IIR";
	reg_name[0xE7078>>2]="GTC_SLAVE_RX_PREV";
	reg_name[0xE707C>>2]="GTC_SLAVE_TX_PREV";
	reg_name[0xE70B0>>2]="GTC_PORT_CTL_B";
	reg_name[0xE70C0>>2]="GTC_PORT_CTL_C";
	reg_name[0xE70D0>>2]="GTC_PORT_CTL_D";//DUPLICATED AT PAGE 840 844
	reg_name[0xE70B4>>2]="GTC_PORT_RX_CURR_B";//DUPLICATED AT PAGE 846
	reg_name[0xE70C4>>2]="GTC_PORT_RX_CURR_C";
	reg_name[0xE70D4>>2]="GTC_PORT_RX_CURR_D";
	reg_name[0xE70B8>>2]="GTC_PORT_TX_CURR_B";
	reg_name[0xE70C8>>2]="GTC_PORT_TX_CURR_C";
	reg_name[0xE70D8>>2]="GTC_PORT_TX_CURR_D";
	reg_name[0xF0008>>2]="TRANS_CONF_A";
	omitted[0xF0008]=0xFF;
	reg_name[0xF000C>>2]="FDI_RX_CTL_A";
	reg_name[0xF0010>>2]="FDI_RX_MISC_A";
	reg_name[0xF0014>>2]="FDI_RX_IIR_A";
	reg_name[0xF0018>>2]="FDI_RX_IMR_A";
	reg_name[0xF0030>>2]="FDI_RX_TUSIZE";
	reg_name[0xF0060>>2]="_TRANSA_CHICKEN1";
	reg_name[0xF0064>>2]="_TRANSA_CHICKEN2";
	reg_name[0x100000>>2]="FENCE_0";
	reg_name[0x100008>>2]="FENCE_1";
	reg_name[0x100010>>2]="FENCE_2";
	reg_name[0x100018>>2]="FENCE_3";
	reg_name[0x100020>>2]="FENCE_4";
	reg_name[0x100028>>2]="FENCE_5";
	reg_name[0x100030>>2]="FENCE_6";
	reg_name[0x100038>>2]="FENCE_7";
	reg_name[0x100040>>2]="FENCE_8";
	reg_name[0x100048>>2]="FENCE_9";
	reg_name[0x100050>>2]="FENCE_10";
	reg_name[0x100058>>2]="FENCE_11";
	reg_name[0x100060>>2]="FENCE_12";
	reg_name[0x100068>>2]="FENCE_13";
	reg_name[0x100070>>2]="FENCE_14";
	reg_name[0x100078>>2]="FENCE_15";
	reg_name[0x100080>>2]="FENCE_16";
	//memset(&omitted[0x100000], 0xff, 0xff);
	reg_name[0x100100>>2]="SNB_DPFC_CTL_SA";
	reg_name[0x101008>>2]="GFX_FLSH_CNTL";
	reg_name[0x120000>>2]="GTFIFODBG";
	reg_name[0x130044>>2]="FORCEWAKE_ACK_HSW";
	reg_name[0x120008>>2]="GT_FIFO_FREE_ENTRIES";
	reg_name[0x120010>>2]="HSW_EDRAM_PRESENT";
	reg_name[0x130040>>2]="LCPLL_CTL";
	reg_name[0x13005C>>2]="DOCUMENTED_UNKNOWN";
	reg_name[0x13805C>>2]="GEN_GT_THREAD_STATUS_REG";
	reg_name[0x138124>>2]="GEN6_PCODE_MAILBOX";
	reg_name[0x138128>>2]="GEN6_PCODE_DATA";
	/*
	reg_name[0x0>>2]="GCAP_VMIN_VMAJ";//0/3/0
	reg_name[0x4>>2]="OUTPAY_INPAY";
	reg_name[0x8>>2]="GCTL";
	reg_name[0xC>>2]="WAKEEN_WAKESTS";
	reg_name[0x10>>2]="GSTS";
	reg_name[0x18>>2]="OUTSTRMPAY_INSTRMPAY";
	reg_name[0x20>>2]="INTCTL";
	reg_name[0x24>>2]="INTSTS";
	reg_name[0x30>>2]="WALCLK";
	reg_name[0x38>>2]="SSYNC";
	reg_name[0x40>>2]="CORBLBASE";
	reg_name[0x44>>2]="CORBUBASE";
	reg_name[0x48>>2]="CORBWP";
	reg_name[0x4C>>2]="CORBCTL_STS_SIZE";
	reg_name[0x50>>2]="RIRBLBASE";//BESPONSE INPUT RING BUFFER LOW BASE
	reg_name[0x54>>2]="RIRBUBASE";
	reg_name[0x58>>2]="RIRBWP_RINTCNT";
	reg_name[0x5C>>2]="RIRBCTL_STS_SIZE";
	reg_name[0x60>>2]="ICOI";
	reg_name[0x64>>2]="IRII";
	reg_name[0x68>>2]="ICS";
	reg_name[0x70>>2]="DPLBASE";
	reg_name[0x80>>2]="SDCTL_STS_1";
	reg_name[0xA0>>2]="SDCTL_STS_2";
	reg_name[0xC0>>2]="SDCTL_STS_3";
	reg_name[0x84>>2]="SDLPIB_1";
	reg_name[0xA4>>2]="SDLPIB_2";
	reg_name[0xC4>>2]="SDLPIB_3";
	reg_name[0x88>>2]="SDCBL_1";
	reg_name[0xA8>>2]="SDCBL_2";
	reg_name[0xC8>>2]="SDCBL_3";
	reg_name[0x8C>>2]="SDLVI_1";
	reg_name[0xAC>>2]="SDLVI_2";
	reg_name[0xCC>>2]="SDLVI_3";
	reg_name[0x90>>2]="SDFIFOP_FMT_1";
	reg_name[0xB0>>2]="SDFIFOP_FMT_2";
	reg_name[0xD0>>2]="SDFIFOP_FMT_3";
	reg_name[0x98>>2]="SDBDPL_1";
	reg_name[0xB8>>2]="SDBDPL_2";
	reg_name[0xD8>>2]="SDBDPL_3";
	reg_name[09C>>2]="SDBDPU_1";
	reg_name[0xBC>>2]="SDBDPU_2";
	reg_name[0xDC>>2]="SDBDPU_3";
	reg_name[0x100C>>2]="EM4";
	reg_name[0x1010>>2]="EM5";
	reg_name[0x1084>>2]="DPIB_1";
	reg_name[0x10A4>>2]="DPIB_2";
	reg_name[0x10C4>>2]="DPIB_3";
	reg_name[0x2030>>2]="WALCLKA";
	reg_name[0x2084>>2]="SDLPIBA_1";
	reg_name[0x20A4>>2]="SDLPIBA_2";
	reg_name[0x20C4>>2]="SDLPIBA_3";

	*/
	
		
		
}
#define VGA_PORT_BASE 	0x3C0
#define REG_AR_INDEX 	(0x3C0 - VGA_PORT_BASE)
#define REG_AR_DATA		(0x3C1 - VGA_PORT_BASE)

#define REG_SEQ_INDEX	(0x3C4 - VGA_PORT_BASE)
#define REG_SEQ_DATA	(0x3C5 - VGA_PORT_BASE)

#define REG_DACMASK		(0x3C6 - VGA_PORT_BASE)

#define REG_DACSTATE_RX	(0x3C7 - VGA_PORT_BASE)
#define REG_DAC_WX		(0x3C8 - VGA_PORT_BASE)
#define REG_DAC_DATA	(0x3C9 - VGA_PORT_BASE)

#define REG_GC_INDEX	(0x3CE - VGA_PORT_BASE)
#define REG_GC_DATA		(0x3CF - VGA_PORT_BASE)


#define REG_ST00		(0x3C2 - VGA_PORT_BASE)
#define REG_ST01		(0x3DA - VGA_PORT_BASE)

#define REG_FCR			(0x3CA - VGA_PORT_BASE)
#define REG_MSR_WRITE	(0x3C2 - VGA_PORT_BASE)
#define REG_MSR_READ	(0x3CC - VGA_PORT_BASE)

#define REG_CRX_INDEX	(0x3D4 - VGA_PORT_BASE)
#define REG_CRX_DATA	(0x3D5 - VGA_PORT_BASE)

const char *vga_seq_name[8];
const char *vga_gc_name[0x1f];
const char *vga_ar_name[0x1f];
const char *vga_port_name[0x400];
const char *vga_crt_name[0x2f];

void init_vga_reg_profile(void)
{
	memset(vga_seq_name, NULL, sizeof(char *) * 8);
	memset(vga_gc_name, NULL, sizeof(char *) * 0x1f);
	memset(vga_ar_name, NULL, sizeof(char *) * 0x1f);
	memset(vga_crt_name, NULL, sizeof(char *) * 0x2f);

	vga_port_name[0x3C0]="ATTRIBUTE_INDEX";
	vga_port_name[0x3C1]="ATTRIBUTE_DATA";
	vga_port_name[0x3C2]="INPUT_STATUS_00";
	vga_port_name[0x3C4]="SEQUENCER_INDEX";
	vga_port_name[0x3C5]="SEQUENCER_DATA";
	vga_port_name[0x3C6]="DAC_MASK";
	vga_port_name[0x3C7]="DAC_STATE/READ_INDEX";
	vga_port_name[0x3C8]="DAC_WRITE_INDEX";
	vga_port_name[0x3C9]="DAC_DATA";
	vga_port_name[0x3CE]="GRAPHIC_CONTROL_INDEX";
	vga_port_name[0x3CF]="GRAPHIC_CONTROL_DATA";
	
	vga_seq_name[0]="'Sequencer Reset";
	vga_seq_name[1]="'clocking mode";
	vga_seq_name[2]="'Plane Mask";
	vga_seq_name[3]="'Character Font";
	vga_seq_name[4]="'Memory Mode Register";
	vga_seq_name[5]="'???";
	vga_seq_name[6]="'???";
	vga_seq_name[7]="'Horizontal Character Counter Reset";

	vga_gc_name[0]="'set/reset Register";
	vga_gc_name[1]="'enable set/reset register";
	vga_gc_name[2]="'color compare register";
	vga_gc_name[3]="'data rotate register";
	vga_gc_name[4]="'read plane select register";
	vga_gc_name[5]="'graphic model register";
	vga_gc_name[6]="'gc misc register";
	vga_gc_name[7]="'color dont care register";
	vga_gc_name[8]="'bit mask register";
	vga_gc_name[0x10]="'address mapping";
	vga_gc_name[0x11]="'page selector";
	vga_gc_name[0x18]="'software flags";

	int i;
	for (i=0; i<=0xf; i++)
		vga_ar_name[i]="'palette Register";
	vga_ar_name[0x10]="'mode control register";
	vga_ar_name[0x12]="'memory plane enable register";
	vga_ar_name[0x13]="'horizontal pixel panning register";
	vga_ar_name[0x14]="'color select register";

	vga_crt_name[0x0]="'Horizontal total register";
	vga_crt_name[0x1]="'Horizontal display enable end reg";
	vga_crt_name[0x2]="'HBlanking Start Reg";
	vga_crt_name[0x3]="'HBlanking End Reg";
	vga_crt_name[0x4]="'HSYNC Start Reg";
	vga_crt_name[0x5]="'HSYNC End Reg";
	vga_crt_name[0x6]="'VTOTAL Reg";
	vga_crt_name[0x7]="'Overflow reg";
	vga_crt_name[0x8]="'Preset Row Scan Reg";
	vga_crt_name[0x9]="'Maximum Scan Line Reg";
	vga_crt_name[0xa]="'text Cursor Start Reg";
	vga_crt_name[0xb]="'text cursor end reg";
	vga_crt_name[0xc]="'start address high reg";
	vga_crt_name[0xd]="'tstart address low reg";
	vga_crt_name[0xe]="'text cursor location high reg";
	vga_crt_name[0xf]="'text cursor location low reg";
	vga_crt_name[0x10]="'VSYNC start reg";
	vga_crt_name[0x11]="'VSYNC end reg";
	vga_crt_name[0x12]="'Vertical display enable end reg";
	vga_crt_name[0x13]="'offset reg";
	vga_crt_name[0x14]="'underline location reg";
	vga_crt_name[0x15]="'VBLANK start reg";
	vga_crt_name[0x16]="'VBLANK end reg";
	vga_crt_name[0x17]="'CRT mode reg";
	vga_crt_name[0x18]="'line compare reg";
	vga_crt_name[0x22]="'memory read latch data reg";
}
void debug_vga(uint64_t hwaddr, unsigned size, uint64_t data,int iswrite)
{
	static int vga_initialized = 0;
	static unsigned dac_read_index = 0;
	static unsigned dac_write_index = 0;
	static unsigned crt_index = 0;
	static unsigned ar_index = 0;
	static unsigned seq_index = 0;
	static unsigned gc_index = 0;
	if(!vga_initialized){
		init_vga_reg_profile();
		vga_initialized = 1;
	}
	if(hwaddr < 0x400)
	{
		switch (hwaddr) {
			case REG_ST00:
			{
				if(iswrite) {
					printf("VGA_MSR_WRITE data: %x\n",
						data);
				} else {
					printf("VGA %s %s data:%x\n",
						iswrite?"write":"read",
						vga_port_name[REG_ST00],
						data);
				}
				break;
			}
			case REG_ST01:
			{
				if(iswrite) {
					printf("VGA_FCR_WRITE data:%x\n",
						data);
				} else {
					printf("VGA_ST01_READ data:%x\n",
						data);
				}
				break;
			}
			case REG_SEQ_INDEX:
			{
				uint8_t seq_data = (data >> (4 *(size - 1)));
				uint8_t index = data & 0x00ff;
				seq_index = index;
				if(size >=2)
					printf("VGA_SEQ %s 0x%x %s data:%x\n",
						iswrite?"write":"read",
						index,
						vga_seq_name[index],
						seq_data);
				break;
			}
			case REG_GC_INDEX:
			{
				uint8_t gc_data = (data >> (4 *(size - 1)));
				uint8_t index = data & 0x00ff;
				gc_index = index;
				if(size >=2)
				printf("VGA_GC %s 0x%x %s data:%x\n",
					iswrite?"write":"read",
					index,
					vga_gc_name[index],
					gc_data);
				break;
			}
			case REG_CRX_INDEX:
			{
				uint8_t crx_data = (data >> (4 *(size - 1)));
				uint8_t index = data & 0x00ff;
				crt_index = data;
				if(size >=2)
					printf("VGA_CRT %s 0x%x %s data:%x\n",
						iswrite?"write":"read",
						index,
						vga_crt_name[index],
						crx_data);
				break;
			}
			case REG_AR_INDEX:
			{
				uint8_t ar_data = (data >> (4 *(size - 1)));
				uint8_t index = data & 0x00ff;
				ar_index = index;
				printf("VGA_AR %s 0x%x %s data:%x\n",
					iswrite?"write":"read",
					index,
					vga_ar_name[index],
					ar_data);
				break;
			}
			case REG_DACSTATE_RX:
			{	
				dac_read_index = data;
				break;
			}
			case REG_DAC_WX:
			{
				dac_write_index = data;
				break;
			}
			case REG_CRX_DATA:
			{
				printf("VGA_AR %s 0x%x %s data:%x\n",
					iswrite?"write":"read",
					crt_index,
					vga_crt_name[ar_index],
					data);
				break;
			}
			case REG_SEQ_DATA:
			{
				printf("VGA_AR %s 0x%x %s data:%x\n",
					iswrite?"write":"read",
					seq_index,
					vga_seq_name[seq_index],
					data);
				break;
			}
			case REG_AR_DATA:
			{

				printf("VGA_AR %s 0x%x %s data:%x\n",
					iswrite?"write":"read",
					index,
					vga_ar_name[ar_index],
					data);
				break;
			}
			case REG_DAC_DATA:
			{
				printf("VGA_DAC %s %x color data:%x\n",
					iswrite?"write":"read",
					iswrite?dac_write_index:dac_read_index,
					data);
				break;
			}
		
			case REG_MSR_READ:
			{
				printf("VGA_MSR_READ data: %x\n", 
					data);
				break;
			}
			default:
				printf("hwaddr: %x, data %x\n", hwaddr, data);
		}
			
	
	}
}

void debug_igd_mmio(uint64_t hwaddr, unsigned size, uint64_t data,int iswrite)
{
	static unsigned char initialized = 0;
	static uint64_t last;
	static int last_iswrite;

	if(!initialized) {
		init_reg_profile();
		printf("init!\n");
		initialized = 1;
		last = ~0x1;
		last_iswrite = -1;
	}
	
	if(hwaddr <= 0x13ffff) {
		if(last == hwaddr && last_iswrite == iswrite){
			supressed[hwaddr] += 1;
			return;
		}else if (supressed[last] > 2){
		
			printf("supressed %u %s to 0x%x %s\n", 
				supressed[last],
				last_iswrite?"write":"read",
				last,
				reg_name[last>>2]);
			supressed[last] = 0;
		}
		
		if(!omitted[hwaddr]) {
			if(reg_name[hwaddr>>2] != NULL){
				last = hwaddr;
				last_iswrite = iswrite;
				if( supressed[hwaddr] <=2 ) {

					printf("\033[1;34mIGD %s @0x%x %s data %lx\033[0m\n", 
						iswrite?"write":"read",
						hwaddr,
						reg_name[hwaddr>>2],
						data);
				}
	
			} else {
				printf("\033[0;34mIGD %s @0x%x UNKNOWN_REGISTER data %lx\033[0m\n", 
					iswrite?"write":"read",
					hwaddr,
					data);		
			}
			
		}
		
	} else if (hwaddr >=0x140000 && hwaddr <= 0x17ffff ) {
			printf("\033[1;35mIGD %s MCHBAR @0x%x data %lx\033[0m\n",
			iswrite?"write":"read",
			hwaddr,
			data);
	}else {
		//printf("\033[1;36mIGD %s GTT @0x%x data %lx\033[0m\n",
		//	iswrite?"write":"read",
		//	hwaddr,
		//	data);
	}
}

#endif
