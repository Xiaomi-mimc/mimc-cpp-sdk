## Version 1.0.0 (2019-09-18)

### Added:
- 重连FE，支持指数退避，默认重链间隔为10s
- 对外提供可控接口：
    - void setBaseOfBackoffWhenConnectFe(TimeUnit timeUnit, long base);
    - void setCapOfBackoffWhenConnectFe(TimeUnit timeUnit, long cap);
    - void mimc_set_base_of_backoff_When_connect_fe(user_t *user, time_unit_t time_unit, long base);
    - void mimc_set_cap_of_backoff_When_connect_fe(user_t *user, time_unit_t time_unit, long cap);
（C）

### Changed:
- void mimc_set_base_of_backoff(user_t *user, time_unit_t time_unit, long base)
更改为：void mimc_set_base_of_backoff_When_fetch_token(user_t *user, time_unit_t time_unit, long base)
- void mimc_set_cap_of_backoff(user_t *user, time_unit_t time_unit, long cap)
更改为：void mimc_set_cap_of_backoff_When_fetch_token(user_t *user, time_unit_t time_unit, long cap)
- Backoff* backoffWhenFetchToken
更改为：
Backoff backoffWhenFetchToken;
- Backoff* getBackoffWhenFetchToken()
更改为：Backoff& getBackoffWhenFetchToken()

## Version 1.0.1 (2019-09-25)

### Added:
- 增加online通道
- 对外发送online消息接口：
    void mimc_send_online_message(user_t* user,const char* to_appaccount,const char *payload, const int payload_len, const char *biz_type, const bool is_store, char *result, int result_len)；
- 接受online消息回调接口：
    void (* handle_online_message)(const mimc_message_t* packets)；

---
---
> ## Version x.x.x (y-m-d)
> ### Added:
> ### Changed:
> ### Fixed:
> ### Removed:
> ### Deprecated:

---
