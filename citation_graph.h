#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>
#include <iostream>
using namespace std;

class PublicationAlreadyCreated : public std::exception {
	const char* what() const noexcept {
		return "PublicationAlreadyCreated";
	}
};

class PublicationNotFound : public std::exception {
	const char* what() const noexcept {
		return "PublicationNotFound";
	}
};

class TriedToRemoveRoot : public std::exception {
	const char* what() const noexcept {
		return "TriedToRemoveRoot";
	}
};

template <typename Publication>
class CitationGraph {
private:
	using id_type = typename Publication::id_type;

	struct Node {
		Publication pub;
		std::set<id_type> par;
		std::set<id_type> chi;
		Node(const id_type &p)
			: pub(p) {}
	};

	std::weak_ptr<Node> root;
	std::map<id_type, std::shared_ptr<Node>> nodes;

	void createHelper(const id_type &nw_node, std::set<id_type> &nw_parents,
	                  const std::vector<id_type> &parent_ids, size_t cur_par_id) {
		auto &cur_par = parent_ids[cur_par_id];
		auto &parent_children = nodes.find(cur_par)->second.get()->chi;
		nw_parents.emplace(cur_par);

		// for adding current connection
		typename std::set<id_type>::iterator it;
		try {
			it = parent_children.emplace(nw_node).first;
		}
		catch (...) {
			throw;
		}

		if (cur_par_id + 1 < parent_ids.size()) {
			// for adding other connections
			try {
				createHelper(nw_node, nw_parents, parent_ids, cur_par_id + 1);
			}
			catch (...) {
				parent_children.erase(it);
				throw;
			}
		}
	}

	void find_lost(const id_type &deleted, const id_type &cur, std::map<id_type, size_t> &par_erased,
				   std::vector<typename std::map<id_type, std::shared_ptr<Node>>::iterator> &to_erase,
				   std::vector<std::pair<std::set<id_type>*, typename std::set<id_type>::iterator>> &fam_erase) {
		par_erased[cur]++;
		if (par_erased[cur] == nodes[cur].get()->par.size()) {
			to_erase.push_back(nodes.find(cur));
			for (auto it : nodes[cur].get()->par) {
				fam_erase.emplace_back(&nodes[it].get()->chi, nodes[it].get()->chi.find(cur));
			}
			for (auto it : nodes[cur].get()->chi) {
				fam_erase.emplace_back(&nodes[it].get()->par, nodes[it].get()->par.find(cur));
			}
			for (auto it : nodes[cur].get()->chi) {
				find_lost(deleted, it, par_erased, to_erase, fam_erase);
			}
		}
	}

public:
	// Tworzy nowy graf. Tworzy także węzeł publikacji o identyfikatorze stem_id.
	CitationGraph(const id_type &stem_id) {
		auto nd = nodes.emplace(stem_id, std::make_shared<Node>(stem_id)).first->second;
		root = nd;
	}

	// Konstruktor przenoszący i przenoszący operator przypisania. Powinny być
	// noexcept.
	CitationGraph(CitationGraph<Publication> &&other) noexcept {
		root = move(other.root);
		nodes = move(other.nodes);
	}

	CitationGraph<Publication>& operator=(CitationGraph<Publication> &&other) noexcept {
		nodes.clear();
		root.reset();
		root = move(other.root);
		nodes = move(other.nodes);
		return *this;
	}

	// Próba użycia konstruktora kopiującego lub kopiującego operatora przypisania
	// dla obiektów klasy CitationGraph powinna zakończyć się błędem kompilacji.
	CitationGraph(CitationGraph<Publication>&) = delete;
	CitationGraph<Publication>& operator=(CitationGraph<Publication>&) = delete;

	// Zwraca identyfikator źródła. Metoda ta powinna być noexcept wtedy i tylko
	// wtedy, gdy metoda Publication::get_id jest noexcept. Zamiast pytajnika należy
	// wpisać stosowne wyrażenie.
	id_type get_root_id() const noexcept(noexcept(root.lock().get()->pub.get_id())) {
		return root.lock().get()->pub.get_id();
	}

	// Zwraca listę identyfikatorów publikacji cytujących publikację o podanym
	// identyfikatorze. Zgłasza wyjątek PublicationNotFound, jeśli dana publikacja
	// nie istnieje.
	std::vector<id_type> get_children(const id_type &id) const {
		if (!exists(id))
			throw PublicationNotFound();
		auto &chi = nodes.find(id)->second.get()->chi;
		return std::vector<id_type>(chi.begin(), chi.end());
	}

	// Zwraca listę identyfikatorów publikacji cytowanych przez publikację o podanym
	// identyfikatorze. Zgłasza wyjątek PublicationNotFound, jeśli dana publikacja
	// nie istnieje.
	std::vector<id_type> get_parents(const id_type &id) const {
		if (!exists(id))
			throw PublicationNotFound();
		auto &par = nodes.find(id)->second.get()->par;
		return std::vector<id_type>(par.begin(), par.end());
	}

	// Sprawdza, czy publikacja o podanym identyfikatorze istnieje.
	bool exists(const id_type &id) const {
		return nodes.find(id) != nodes.end();
	}

	// Zwraca referencję do obiektu reprezentującego publikację o podanym
	// identyfikatorze. Zgłasza wyjątek PublicationNotFound, jeśli żądana publikacja
	// nie istnieje.
	Publication& operator[](const id_type &id) const {
		if (!exists(id))
			throw PublicationNotFound();
		return nodes.find(id)->second.get()->pub;
	}

	// Tworzy węzeł reprezentujący nową publikację o identyfikatorze id cytującą
	// publikacje o podanym identyfikatorze parent_id lub podanych identyfikatorach
	// parent_ids. Zgłasza wyjątek PublicationAlreadyCreated, jeśli publikacja
	// o identyfikatorze id już istnieje. Zgłasza wyjątek PublicationNotFound, jeśli
	// któryś z wyspecyfikowanych poprzedników nie istnieje.
	void create(const id_type &id, const id_type &parent_id) {
		create(id, std::vector<id_type>{parent_id});
	}

	void create(const id_type &id, const std::vector<id_type> &parent_ids) {
		if (exists(id))
			throw PublicationAlreadyCreated();
		for (auto &i : parent_ids)
			if (!exists(i))
				throw PublicationNotFound();
		if (parent_ids.empty())
			throw PublicationNotFound();
			
		auto it = nodes.emplace(id, std::make_shared<Node>(id)).first;

		try {
			createHelper(id, it->second.get()->par, parent_ids, 0);
		}
		catch (...) {
			nodes.erase(it);
			throw;
		}
	}

	// Dodaje nową krawędź w grafie cytowań. Zgłasza wyjątek PublicationNotFound,
	// jeśli któraś z podanych publikacji nie istnieje.
	void add_citation(const id_type &child_id, const id_type &parent_id) {
		if (!exists(child_id) || !exists(parent_id))
			throw PublicationNotFound();
		auto &child_parents = nodes.find(child_id)->second.get()->par;
		auto &parent_children = nodes.find(parent_id)->second.get()->chi;
		
		auto it = child_parents.emplace(parent_id);
		try {
			parent_children.emplace(child_id);
		}
		catch (...) {
			if (it.second)
				child_parents.erase(it.first);
			throw;
		}
	}

	// Usuwa publikację o podanym identyfikatorze. Zgłasza wyjątek
	// PublicationNotFound, jeśli żądana publikacja nie istnieje. Zgłasza wyjątek
	// TriedToRemoveRoot przy próbie usunięcia pierwotnej publikacji.
	void remove(const id_type &id) {
		if (!exists(id))
			throw PublicationNotFound();
		if (id == get_root_id())
			throw TriedToRemoveRoot();
		std::map<id_type, size_t> par_erased;
		std::vector<typename std::map<id_type, std::shared_ptr<Node>>::iterator> to_erase;
		std::vector<std::pair<std::set<id_type>*, typename std::set<id_type>::iterator>> fam_erase;
		par_erased[id] = nodes[id].get()->par.size() - 1;
		find_lost(id, id, par_erased, to_erase, fam_erase);
		for (auto fam : fam_erase){
			(*fam.first).erase(fam.second);
		}
		for (auto toer : to_erase){
			nodes.erase(toer);
		}
	}
};
