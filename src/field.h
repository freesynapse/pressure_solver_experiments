
#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string.h>
#include <memory>

//
#define ASSERT_SZ(f) assert(f.size() == m_n)
#define ASSERT_SZ_PTR(f) assert(f->size() == m_n)


//
template<typename T>
class Field
{
public:

    //
    Field() {}
    Field(uint32_t _cell_count) : m_n(_cell_count) { new_(); }
    Field(const glm::ivec2 &_shape) : m_n(_shape.x * _shape.y) { new_(); } 
    ~Field() { if (m_data) delete[] m_data; if (m_swap) delete[] m_swap; }

    //
    void set(T &_val, bool _back_buffer=false)
    {
        glm::vec2 *p = (_back_buffer ? m_swap : m_data);
        for (uint32_t i = 0; i < m_n; i++)
            p[i] = _val;
    }

    //
    void clear(bool _back_buffer=false)
    {
        if (!_back_buffer)  memset(m_data, 0, m_sz_bytes);
        else                memset(m_swap, 0, m_sz_bytes);
    }

    //
    void swap()
    {
        T *t = m_data;
        m_data = m_swap;
        m_swap = m_data;
    }

    //
    T *data() { return m_data; }
    T *backBuffer() { return m_swap; }
    uint32_t size() { return m_n; }
    uint32_t size_bytes() { return m_sz_bytes; }

    //
    void copyFrom(Field &_f) { ASSERT_SZ(_f); memcpy(m_data, _f.data(), m_sz_bytes); }
    void copyFrom(Field *_f) { ASSERT_SZ_PTR(_f); memcpy(m_data, _f->data(), m_sz_bytes); }
    void copyFrom(std::shared_ptr<Field> _f) { ASSERT_SZ_PTR(_f.get()); memcpy(m_data, _f->data(), m_sz_bytes); }


private:
    void new_()
    {
        m_data = new T[m_n];
        m_swap = new T[m_n];
        m_sz_bytes = sizeof(T) * m_n;
    }

private:
    T *m_data = nullptr;
    T *m_swap = nullptr;

    uint32_t m_n;
    uint32_t m_sz_bytes;
    
};

using Field1D = Field<float>;
using Field2D = Field<glm::vec2>;


