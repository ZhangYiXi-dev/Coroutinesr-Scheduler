#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <list>
#include "common.h"
#include "zyx_thread.h"
#include "fiber.h"

namespace zyx
{
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        //如果是false则没设置主协程，否则会设置主协程
        Scheduler(int thread_size=1,bool use_caller=false); 
        ~Scheduler();
         /**
         * @brief 返回当前协程调度器
         */
        static Scheduler* GetThis();
        /**
         * @brief 返回当前协程调度器的调度协程
         */
        static Fiber* GetMainFiber();
        /**
        * @brief 设置当前的协程调度器
        */
        void setThis();
         /**
         * @brief 协程调度函数
         */
        void run();

        //调度器启动
        void start();
        /**
         * @brief 停止协程调度器
         */
        void stop();
        /**
         * @brief 调度协程
         * @param[in] fc 协程或函数
         * @param[in] thread 协程执行的线程id,-1标识任意线程
         */
        template<class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1) {
            bool need_tickle = false;
            {
                m_mutex.lock();
                need_tickle = scheduleNoLock(fc, thread);
                m_mutex.unlock();
            }

            if(need_tickle) {
                tickle();
            }
        }
    private:
           /**
         * @brief 协程/函数/线程组
         */
        struct FiberAndThread {
            /// 协程
            Fiber::ptr fiber;
            /// 协程执行函数
            std::function<void()> cb;
            /// 线程id
            int thread;

        /**
         * @brief 构造函数
         * @param[in] f 协程
         * @param[in] thr 线程id
         */
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr) {
            fiber.swap(*f);
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数
         * @param[in] thr 线程id
         */
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr) {
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
            cb.swap(*f);
        }

        /**
         * @brief 无参构造函数
         */
        FiberAndThread()
            :thread(-1) {
        }

        /**
         * @brief 重置数据
         */
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) 
    {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
    protected:
        /**
         * @brief 通知协程调度器有任务了
         */
        virtual void tickle();
        /**
         * @brief 返回是否可以停止
         */
        virtual bool stopping();
        /**
         * @brief 协程无任务可调度时执行idle协程
         */
        virtual void idle();
    private:
        Mutex m_mutex; //互斥锁
        int m_threadCount=0; //线程数量
        std::vector<Thread::ptr> m_threads; //线程池
        std::list<FiberAndThread> m_fibers; //协程队列
        Fiber::ptr m_rootFiber; //调度其他协程的主协程
        bool m_stopping=true;
        pid_t m_rootThread = 0;/// 主线程id(use_caller)
    };
}
#endif