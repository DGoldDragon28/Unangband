---
---
<ul>
  {% for post in site.posts %}
    <li>
      {{ post.date | date_to_rfc822 }}
      {{ post.excerpt }} 
    </li>
  {% endfor %}
</ul>
