#ifdef skip

#include <functional> 
#include <list>
#include <mutex>
#include <vector>

template<typename Key, typename Value, typename Hash=std::hash<Key> >
class threadsafe_map {
	/*
		һ����ϣ���ɶ��Ͱ��ɣ�ÿ��Ͱ��Ź�ϣֵ��ͬ�����ݣ�ÿ��Ͱ��һ��������
	*/
private:
	/*
		Ͱ����Ҫ������һ��std::list<bucket_data>�����key��ͬ�����ݣ�ͬһ��ϣֵ����ϣ��ͻ����������
		ÿһ��bucket_data����һ��key_value��
		Key����Ͱ�Ĺ�ϣֵ
		Value������Ҫ�洢������
	*/
	class bucket_type {
	private:
		typedef std::pair<Key, Value> bucket_value;
		typedef std::list<bucket_value> bucket_data;
		typedef typename bucket_data::iterator bucket_iterator;

		bucket_data data;
		mutable boost::share_mutex mutex;

		bucket_iterator find_entry_for(Key const& key) const {// ��������ϣֵ�����Һ�����Ӧ���޸ĳ�Ա��������const
			// find������data�е�ĳ��Ԫ�ر���
			// find_if������data�е�ĳ��Ԫ�������е�������data��Ԫ�ؼ�Ϊ����ɵ��ö��������
			// ���߷���������ͬ
			return std::find_if(data.begin(), data.end(), [&](bucket_value const& item) {return item.first == key})
		}

	public:
		Value value_for(Key const& key, Value const& default_value) const {
			boost::shared_lock<boost::shared_mutex> lock(mutex);
			bucket_iterator const found_entry = find_entry_for(key);
			return (found_entry ? default_value : found_entry.secend)
		}

		void add_or_update_mapping(Key const& key, Value const& value) {
			std::unique_lock<boost::shared_mutex> lock(mutex);
			bucket_iterator const found_entry = find_entry_for(key);
			if (found_entry == data.end()) {
				data.push_back(bucket_value(value, key));
			}
			else {
				found_entry->secend = value;
			}
		}

		void remove_mapping(Key const& key) {
			std::unique_lock <boost::shared_mutex> lock(mutex);
			bucket_iterator const found_entry = find_entry_for(key);
			if (found_entry != data.end()) {
				data.erase(found_entry);
			}
		}
	};

	std::vector<std::unique_ptr<bucket_type> > buckets;
	Hash hasher;

	bucket_type& get_bucket(Key const& key) const {
		// hasher: ���е�key����->size_t����
		//ͨ��%���������Ĺ�ϣֵ����
		std::size_t const bucket_index = hasher(key) % buckets.size();
		return *buckets[bucket_index];
	}
public:
	typedef Key key_type;
	typedef Value mapped_type;
	typedef Hash hash_type;

	threadsafe_map(unsigned num_buckets = 19, Hash const& hasher_ = Hash()) :
		buckets(num_buckets), hasher(hasher_){
		for ()
		
	}
};

#endif