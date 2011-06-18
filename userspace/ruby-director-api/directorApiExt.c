#include "ruby.h"
#include "director-api.h"

static VALUE module, processingLoopMethod;

static void ruby_npm_check_callback(pid_t pid, uid_t uid, int is_guest, const char* name, struct rusage *rusage, int* decision, int* decision_value);
static void ruby_npm_check_full_callback(pid_t pid, uid_t uid, int is_guest, const char* name, char** args, char** envp, int* decision, int* decision_value);
static void ruby_node_connected_callback(char* address, int slot_index, int auth_data_size, const char* auth_data, int* accept );
static void ruby_node_disconnected_callback(int slot_index, int slot_type, int reason);
static void ruby_immigrate_request_callback(uid_t uid, int slot_index, const char* name, int* accept);
static void ruby_immigration_confirmed_callback(uid_t uid, int slot_index, const char* name, pid_t local_pid, pid_t remote_pid);
static void ruby_task_exitted_callback(pid_t pid, int exit_code, struct rusage *rusage);
static void ruby_task_forked_callback(pid_t pid, pid_t ppid);
static void ruby_migrated_home_callback(pid_t pid);
static void ruby_emigration_failed_callback(pid_t pid);
static void ruby_user_message_received_callback(int node_id, int slot_type, int slot_index, int user_data_size, char* user_data);

static VALUE ruby_rusage(struct rusage *rusage)
{
	if (rusage == NULL)
		return Qnil;

	VALUE rb_rusage = rb_hash_new();
	VALUE rb_ru_utime = rb_hash_new();
	VALUE rb_ru_stime = rb_hash_new();

	rb_hash_aset(rb_ru_utime, rb_str_new2("tv_sec"), INT2FIX(rusage->ru_utime.tv_sec));
	rb_hash_aset(rb_ru_utime, rb_str_new2("tv_usec"), INT2FIX(rusage->ru_utime.tv_usec));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_utime"), rb_ru_utime);

	rb_hash_aset(rb_ru_stime, rb_str_new2("tv_sec"), INT2FIX(rusage->ru_stime.tv_sec));
	rb_hash_aset(rb_ru_stime, rb_str_new2("tv_usec"), INT2FIX(rusage->ru_stime.tv_usec));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_stime"), rb_ru_stime);

	rb_hash_aset(rb_rusage, rb_str_new2("ru_maxrss"), INT2FIX(rusage->ru_maxrss));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_ixrss"), INT2FIX(rusage->ru_ixrss));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_idrss"), INT2FIX(rusage->ru_idrss));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_isrss"), INT2FIX(rusage->ru_isrss));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_minflt"), INT2FIX(rusage->ru_minflt));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_majflt"), INT2FIX(rusage->ru_majflt));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_nswap"), INT2FIX(rusage->ru_nswap));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_inblock"), INT2FIX(rusage->ru_inblock));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_oublock"), INT2FIX(rusage->ru_oublock));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_msgsnd"), INT2FIX(rusage->ru_msgsnd));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_msgrcv"), INT2FIX(rusage->ru_msgrcv));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_nsignals"), INT2FIX(rusage->ru_nsignals));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_nvcsw"), INT2FIX(rusage->ru_nvcsw));
	rb_hash_aset(rb_rusage, rb_str_new2("ru_nivcsw"), INT2FIX(rusage->ru_nivcsw));

	return rb_rusage;
}

static VALUE method_init(VALUE self)
{
	int ret = initialize_director_api();
	if ( ret ) {
		puts("Failed to initialize director api");
		rb_raise(Qnil, "Failed to initialize director api");
	}
	register_npm_check_callback(ruby_npm_check_callback);
	register_npm_check_full_callback(ruby_npm_check_full_callback);
	register_node_connected_callback(ruby_node_connected_callback);
	register_node_disconnected_callback(ruby_node_disconnected_callback);
	register_immigration_request_callback(ruby_immigrate_request_callback);
	register_immigration_confirmed_callback(ruby_immigration_confirmed_callback);
	register_task_exitted_callback(ruby_task_exitted_callback);
	register_task_forked_callback(ruby_task_forked_callback);
	register_migrated_home_callback(ruby_migrated_home_callback);
	register_emigration_failed_callback(ruby_emigration_failed_callback);
	register_generic_user_message_callback(ruby_user_message_received_callback);

	rb_iv_set(self, "@npmCallbackTarget", Qnil);
	rb_iv_set(self, "@npmCallbackFunction", Qnil);
	rb_iv_set(self, "@npmFullCallbackTarget", Qnil);
	rb_iv_set(self, "@npmFullCallbackFunction", Qnil);
	rb_iv_set(self, "@nodeConnectedCallbackTarget", Qnil);
	rb_iv_set(self, "@nodeConnectedCallbackFunction", Qnil);
	rb_iv_set(self, "@nodeDisconnectedCallbackTarget", Qnil);
	rb_iv_set(self, "@nodeDisconnectedCallbackFunction", Qnil);	
	rb_iv_set(self, "@immigrateRequestCallbackTarget", Qnil);
	rb_iv_set(self, "@immigrateRequestCallbackFunction", Qnil);
	rb_iv_set(self, "@immigrationConfirmedCallbackTarget", Qnil);
	rb_iv_set(self, "@immigrationConfirmedCallbackFunction", Qnil);	
	rb_iv_set(self, "@taskExittedCallbackTarget", Qnil);
	rb_iv_set(self, "@taskExittedCallbackFunction", Qnil);
	rb_iv_set(self, "@taskForkedCallbackTarget", Qnil);
	rb_iv_set(self, "@taskForkedCallbackFunction", Qnil);	
	rb_iv_set(self, "@migratedHomeCallbackTarget", Qnil);
	rb_iv_set(self, "@migratedHomeCallbackFunction", Qnil);	
	rb_iv_set(self, "@emigrationFailedCallbackTarget", Qnil);
	rb_iv_set(self, "@emigrationFailedCallbackFunction", Qnil);	
	rb_iv_set(self, "@userMessageReceivedCallbackTarget", Qnil);
	rb_iv_set(self, "@userMessageReceivedCallbackFunction", Qnil);
	return self;
}

static void parse_npm_call_result(VALUE result, int* decision, int* decision_value) {
	*decision = DO_NOT_MIGRATE;
	if ( result != Qnil ) {
		struct RArray* resArray;
		Check_Type(result, T_ARRAY);
		resArray = RARRAY(result);
		if ( resArray->len < 1 )
			rb_raise(rb_intern("StandardError"), "Resulting array must contain at least 1 element");

		*decision = FIX2INT(resArray->ptr[0]);
		if ( resArray->len > 1 )
			*decision_value = FIX2INT(resArray->ptr[1]);		

		//printf("Read decision: %d\n", *decision);
	}
}

static void ruby_npm_check_callback(pid_t pid, uid_t uid, int is_guest, const char* name, 
		struct rusage *rusage, int* decision, int* decision_value) 
{
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@npmCallbackFunction");
	callbackTarget = rb_iv_get(self,"@npmCallbackTarget");
	if ( callbackMethod != Qnil ) {
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 5, INT2FIX(pid), 
				INT2FIX(uid), rb_str_new2(name), is_guest ? Qtrue : Qfalse, ruby_rusage(rusage));
	}

//	*decision = DO_NOT_MIGRATE;
//	*decision = REQUIRE_ARGS;
	parse_npm_call_result(callResult, decision, decision_value);
}

static VALUE make_array(char** arr) {
	int i=0;
	VALUE res_array = rb_ary_new();

	while ( arr[i] != NULL ) {
		rb_ary_push(res_array, rb_str_new2(arr[i]));
		i++;
	}

	return res_array;
}

static void ruby_npm_check_full_callback(pid_t pid, uid_t uid, int is_guest, const char* name, char** args, char** envp, int* decision, int* decision_value) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@npmFullCallbackFunction");
	callbackTarget = rb_iv_get(self,"@npmFullCallbackTarget");
	if ( callbackMethod != Qnil ) {
		
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 6, INT2FIX(pid), INT2FIX(uid), rb_str_new2(name), is_guest ? Qtrue : Qfalse, make_array(args), make_array(envp));
	}

	parse_npm_call_result(callResult, decision, decision_value);
}

static void ruby_node_connected_callback(char* address, int slot_index, int auth_data_size, const char* auth_data, int* accept ) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@nodeConnectedCallbackFunction");
	callbackTarget = rb_iv_get(self,"@nodeConnectedCallbackTarget");
	if ( callbackMethod != Qnil ) {
		// Note: We do not use auth_data_size, we assume it will be always a string so we can parse it as such
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 3, rb_str_new2(address), INT2FIX(slot_index), auth_data != NULL ? rb_str_new2(auth_data) : Qnil);
	}

	if ( callResult != Qnil ) {
		*accept = callResult == Qtrue ? 1 : 0;
	} else {
		*accept = 1;
	}
}

static void ruby_node_disconnected_callback(int slot_index, int slot_type, int reason ) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@nodeDisconnectedCallbackFunction");
	callbackTarget = rb_iv_get(self,"@nodeDisconnectedCallbackTarget");
	if ( callbackMethod != Qnil ) {
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 3, INT2FIX(slot_index), INT2FIX(slot_type), INT2FIX(reason));
	}
}


static void ruby_immigrate_request_callback(uid_t uid, int slot_index, const char* name, int* accept ) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@immigrateRequestCallbackFunction");
	callbackTarget = rb_iv_get(self,"@immigrateRequestCallbackTarget");
	if ( callbackMethod != Qnil ) {
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 3,INT2FIX(uid), INT2FIX(slot_index), rb_str_new2(name));
	}

	if ( callResult != Qnil ) {
		*accept = callResult == Qtrue ? 1 : 0;
	} else {
		*accept = 1;
	}
}

static void ruby_immigration_confirmed_callback(uid_t uid, int slot_index, const char* name, pid_t local_pid, pid_t remote_pid) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@immigrationConfirmedCallbackFunction");
	callbackTarget = rb_iv_get(self,"@immigrationConfirmedCallbackTarget");
	if ( callbackMethod != Qnil ) {
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 5,INT2FIX(uid), INT2FIX(slot_index), rb_str_new2(name), INT2FIX(local_pid), INT2FIX(remote_pid));
	}
}

static void ruby_task_exitted_callback(pid_t pid, int exit_code, struct rusage *rusage) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@taskExittedCallbackFunction");
	callbackTarget = rb_iv_get(self,"@taskExittedCallbackTarget");
	if ( callbackMethod != Qnil ) {
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 3, 
				INT2FIX(pid), INT2FIX(exit_code), ruby_rusage(rusage));
	}
}

static void ruby_task_forked_callback(pid_t pid, pid_t ppid) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@taskForkedCallbackFunction");
	callbackTarget = rb_iv_get(self,"@taskForkedCallbackTarget");
	if ( callbackMethod != Qnil ) {
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 2, INT2FIX(pid), INT2FIX(ppid));
	}
}

static void ruby_migrated_home_callback(pid_t pid) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@migratedHomeCallbackFunction");
	callbackTarget = rb_iv_get(self,"@migratedHomeCallbackTarget");
	if ( callbackMethod != Qnil ) {
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 1, INT2FIX(pid));
	}
}

static void ruby_emigration_failed_callback(pid_t pid) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@emigrationFailedCallbackFunction");
	callbackTarget = rb_iv_get(self,"@emigrationFailedCallbackTarget");
	if ( callbackMethod != Qnil ) {
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 1, INT2FIX(pid));
	}
}

static void ruby_user_message_received_callback(int node_id, int slot_type, int slot_index, int user_data_size, char* user_data) {
	VALUE self, selfClass, instanceMethod, callbackTarget, callbackMethod;
	VALUE callResult = Qnil;
	
	selfClass = rb_const_get(rb_cObject, rb_intern("DirectorNetlinkApi"));
	instanceMethod = rb_intern("instance");
	self = rb_funcall(selfClass, instanceMethod, 0);
	callbackMethod = rb_iv_get(self,"@userMessageReceivedCallbackFunction");
	callbackTarget = rb_iv_get(self,"@userMessageReceivedCallbackTarget");
	if ( callbackMethod != Qnil ) {
		callResult = rb_funcall(callbackTarget, rb_to_id(callbackMethod), 4, INT2FIX(slot_type), INT2FIX(slot_index), INT2FIX(user_data_size), rb_str_new(user_data, user_data_size));
	}	
}

/** This can be executed multiple times, if required */
static VALUE method_runDirectorNetlinkProcessingLoop(VALUE self) {
	int netlink_fd = get_netlink_fd();

	while (1) {
		run_processing_callback(0); // Do not allow blocking!
		
		//rb_thread_sleep(1);
		//rb_thread_schedule();		
		rb_thread_wait_fd(netlink_fd);
	}
	//finalize_director_api(); TODO: Where to finalize?

	return Qnil;
}

static VALUE method_registerNpmCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@npmCallbackTarget", callbackTarget);
	rb_iv_set(self, "@npmCallbackFunction", callbackFunc);
}

static VALUE method_registerNpmFullCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@npmFullCallbackTarget", callbackTarget);
	rb_iv_set(self, "@npmFullCallbackFunction", callbackFunc);
}

static VALUE method_registerNodeConnectedCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@nodeConnectedCallbackTarget", callbackTarget);
	rb_iv_set(self, "@nodeConnectedCallbackFunction", callbackFunc);
}

static VALUE method_registerNodeDisconnectedCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@nodeDisconnectedCallbackTarget", callbackTarget);
	rb_iv_set(self, "@nodeDisconnectedCallbackFunction", callbackFunc);
}

static VALUE method_registerImmigrateRequestCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@immigrateRequestCallbackTarget", callbackTarget);
	rb_iv_set(self, "@immigrateRequestCallbackFunction", callbackFunc);
}

static VALUE method_registerImmigrationConfirmedCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@immigrationConfirmedCallbackTarget", callbackTarget);
	rb_iv_set(self, "@immigrationConfirmedCallbackFunction", callbackFunc);
}

static VALUE method_registerTaskExittedCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@taskExittedCallbackTarget", callbackTarget);
	rb_iv_set(self, "@taskExittedCallbackFunction", callbackFunc);
}

static VALUE method_registerTaskForkedCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@taskForkedCallbackTarget", callbackTarget);
	rb_iv_set(self, "@taskForkedCallbackFunction", callbackFunc);
}

static VALUE method_registerMigratedHomeCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@migratedHomeCallbackTarget", callbackTarget);
	rb_iv_set(self, "@migratedHomeCallbackFunction", callbackFunc);
}

static VALUE method_registerEmigrationFailedCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@emigrationFailedCallbackTarget", callbackTarget);
	rb_iv_set(self, "@emigrationFailedCallbackFunction", callbackFunc);
}

static VALUE method_registerUserMessageReceivedCallback(VALUE self, VALUE callbackTarget, VALUE callbackFunc) {
	rb_iv_set(self, "@userMessageReceivedCallbackTarget", callbackTarget);
	rb_iv_set(self, "@userMessageReceivedCallbackFunction", callbackFunc);
}

static VALUE method_sendUserMessage(VALUE self, VALUE targetSlotType, VALUE targetSlotIndex, VALUE dataLength, VALUE data) {
	int targetSlotIndex_value, targetSlotType_value, dataLength_value, res;
	char* data_value;

	targetSlotType_value = FIX2INT(targetSlotType);
	targetSlotIndex_value = FIX2INT(targetSlotIndex);
	dataLength_value = FIX2INT(dataLength);
	data_value = StringValueCStr(data);
	res = send_user_message(targetSlotType_value, targetSlotIndex_value, dataLength_value, data_value);

	return INT2FIX(res);
}

static VALUE netlinkApi;

static void define_npm_result_codes(void) {
	rb_define_const(netlinkApi, "DO_NOT_MIGRATE", INT2FIX(DO_NOT_MIGRATE));
	rb_define_const(netlinkApi, "MIGRATE", INT2FIX(MIGRATE));
	rb_define_const(netlinkApi, "MIGRATE_BACK", INT2FIX(MIGRATE_BACK));
	rb_define_const(netlinkApi, "REQUIRE_ARGS", INT2FIX(REQUIRE_ARGS));
	rb_define_const(netlinkApi, "REQUIRE_ENVP", INT2FIX(REQUIRE_ENVP));
	rb_define_const(netlinkApi, "REQUIRE_ARGS_AND_ENVP", INT2FIX(REQUIRE_ARGS_AND_ENVP));
};



void
Init_directorApi()
{
	VALUE singletonModule;
	rb_require("singleton");
	singletonModule = rb_const_get(rb_cObject, rb_intern("Singleton"));

	netlinkApi = rb_define_class("DirectorNetlinkApi", rb_cObject);
	rb_include_module(netlinkApi, singletonModule);
	//rb_funcall(netlinkApi, rb_intern("included"), 1, singletonModule);
	rb_funcall(singletonModule, rb_intern("included"), 1, netlinkApi);
	define_npm_result_codes();
	rb_define_method(netlinkApi, "initialize", method_init, 0);
	rb_define_method(netlinkApi, "registerNpmCallback", method_registerNpmCallback, 2);
	rb_define_method(netlinkApi, "registerNpmFullCallback", method_registerNpmFullCallback, 2);
	rb_define_method(netlinkApi, "registerNodeConnectedCallback", method_registerNodeConnectedCallback, 2);
	rb_define_method(netlinkApi, "registerNodeDisconnectedCallback", method_registerNodeDisconnectedCallback, 2);
	rb_define_method(netlinkApi, "registerImmigrateRequestCallback", method_registerImmigrateRequestCallback, 2);
	rb_define_method(netlinkApi, "registerImmigrationConfirmedCallback", method_registerImmigrationConfirmedCallback, 2);
	rb_define_method(netlinkApi, "registerTaskExittedCallback", method_registerTaskExittedCallback, 2);
	rb_define_method(netlinkApi, "registerTaskForkedCallback", method_registerTaskForkedCallback, 2);
	rb_define_method(netlinkApi, "registerEmigrationFailedCallback", method_registerEmigrationFailedCallback, 2);
	rb_define_method(netlinkApi, "registerMigratedHomeCallback", method_registerMigratedHomeCallback, 2);
	rb_define_method(netlinkApi, "registerUserMessageReceivedCallback", method_registerUserMessageReceivedCallback, 2);
	rb_define_method(netlinkApi, "sendUserMessage", method_sendUserMessage, 4);	
	rb_define_method(netlinkApi, "runProcessingLoop", method_runDirectorNetlinkProcessingLoop, 0);	
}

