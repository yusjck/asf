#ifndef IPCSERVER_H
#define IPCSERVER_H

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/thread.h>

#include <vector>
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

class IpcServer : public IpcConnectionManager
{
public:
	bool Start(uint16_t port, IpcConnectionFactory *connectionFactory);
#ifndef _WIN32
	bool StartByPipeMode(const char *name, IpcConnectionFactory *connectionFactory);
#endif
	void Stop();
	void Wait();
	bool IsRunning();

	IpcServer();
	virtual ~IpcServer();

private:
	struct event_base *m_evbase;
	struct evconnlistener *m_listener;
	THREAD_T m_dispatchThread;
	bool m_running;
	std::vector<IpcConnection *> m_connList;
	IpcConnectionFactory *m_connectionFactory;

	bool StartListen(const struct sockaddr *sa, int socklen);
	void OnAccept(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sock, int socklen);
	virtual void CloseConnection(IpcConnection *conn);

	static THREAD_FN DispatchThread(void *arg);
	static void accept_cb(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sock, int socklen, void *arg);
	static void signal_cb(evutil_socket_t sig, short events, void *user_data);
};

#endif
