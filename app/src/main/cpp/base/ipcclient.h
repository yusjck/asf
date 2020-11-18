#ifndef IPCCLIENT_H
#define IPCCLIENT_H

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event_struct.h>
#include <event2/util.h>

#include <string>
#include <thread>
#include "buffer.h"
#include "ipcconnection.h"

#ifdef EVENT__HAVE_PTHREADS
#define THREAD_T pthread_t
#define THREAD_FN void *
#define THREAD_RETURN() return (NULL)
#define THREAD_START(threadvar, fn, arg) pthread_create(&(threadvar), NULL, fn, arg)
#define THREAD_JOIN(th) pthread_join(th, NULL)
#else
#define THREAD_T HANDLE
#define THREAD_FN DWORD __stdcall
#define THREAD_RETURN() return (0)
#define THREAD_START(threadvar, fn, arg) threadvar = CreateThread(NULL,0,fn,(arg),0,NULL)
#define THREAD_JOIN(th) do { \
							WaitForSingleObject(th, INFINITE); \
							CloseHandle(th); \
						} while (0)
#endif

class IpcClient : public IpcConnectionManager, public IpcConnection
{
public:
	bool Connect(const char *name);
#ifndef _WIN32
	bool ConnectByPipeMode(const char *name);
#endif

	IpcClient();
	virtual ~IpcClient();

protected:
	virtual void OnConnectFail() {};

private:
	struct event_base *m_evbase;
	struct bufferevent *m_bevSocket;
	struct event m_connectTimeout;
	THREAD_T m_dispatchThread;

	bool StartConnect(const struct sockaddr *sa, int socklen);
	void CloseConnection(IpcConnection *conn);
	void OnEvent(struct bufferevent *bev, short event);

	static THREAD_FN DispatchThread(void *arg);
	static void connect_event_cb(struct bufferevent *bev, short event, void *arg);
	static void connect_timeout_cb(evutil_socket_t fd, short event, void *arg);
};

#endif